#include <esp_err.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <lwip/sockets.h>
#include <sdkconfig.h>
#include <stdint.h>

#include "artnet_client_tracking.h"
#include "artnet_const.h"
#include "button.h"
#include "dmxbox_artnet.h"
#include "dmxbox_const.h"
#include "dmxbox_led.h"
#include "dmxbox_storage.h"
#include "hashmap.h"
#include "wifi.h"

static const char *TAG = "artnet";

// Spec: http://www.artisticlicence.com/WebSiteMaster/User%20Guides/art-net.pdf

#define SHORT_NAME "DMX Box"
#define LONG_NAME "Art-Net -> DMX converter"
#define LOG_DMX_DATA false
static const uint32_t POLL_REPLY_INTERVAL = 10 * 1000;

typedef struct dmxbox_artnet_listener_context {
  char *name;
  char *receive_task_name;
  char *poll_reply_task_name;
  EventBits_t connected_bit;
  EventBits_t disconnected_bit;
  esp_netif_t *interface;
  int sock;
  uint8_t packet_buffer[MAX_PACKET_SIZE];
} dmxbox_artnet_listener_context_t;

typedef struct dmxbox_artnet_universe {
  struct dmxbox_artnet_universe *next;
  uint16_t address;
  uint8_t data[DMX_CHANNEL_COUNT];
} dmxbox_artnet_universe_t;

static dmxbox_artnet_universe_t *dmxbox_artnet_universe_alloc() {
  return calloc(1, sizeof(dmxbox_artnet_universe_t));
}

void dmxbox_artnet_universe_free(dmxbox_artnet_universe_t *data) { free(data); }

#define MAX_UNIVERSES_IN_POLL_REPLY 4

typedef struct dmxbox_artnet_universe_advertisement {
  struct dmxbox_artnet_universe_advertisement *next;
  uint8_t bind_index;
  uint8_t net;
  uint8_t subnet;
  uint8_t universes_low[MAX_UNIVERSES_IN_POLL_REPLY];
  uint8_t universe_count;
} dmxbox_artnet_universe_advertisement_t;

static dmxbox_artnet_universe_advertisement_t *
dmxbox_artnet_universe_advertisement_alloc() {
  return calloc(1, sizeof(dmxbox_artnet_universe_advertisement_t));
}

void dmxbox_artnet_universe_advertisement_free(
    dmxbox_artnet_universe_advertisement_t *data
) {
  free(data);
}

uint16_t dmxbox_artnet_native_universe_address = 0;
static dmxbox_artnet_universe_t *universes_head = NULL;
static dmxbox_artnet_universe_t *dmxbox_artnet_native_universe = NULL;

static dmxbox_artnet_universe_advertisement_t *universe_advertisements_head =
    NULL;

portMUX_TYPE dmxbox_artnet_spinlock = portMUX_INITIALIZER_UNLOCKED;

dmxbox_artnet_listener_context_t ap_context = {
    .name = "AP",
    .receive_task_name = "ArtNet AP receive loop",
    .poll_reply_task_name = "ArtNet AP poll reply",
    .connected_bit = dmxbox_wifi_ap_sta_connected,
    .disconnected_bit = dmxbox_wifi_ap_stopped,
    .interface = NULL,
    .sock = -1,
};

dmxbox_artnet_listener_context_t sta_context = {
    .name = "STA",
    .receive_task_name = "ArtNet STA receive loop",
    .poll_reply_task_name = "ArtNet STA poll reply",
    .connected_bit = dmxbox_wifi_sta_connected,
    .disconnected_bit = dmxbox_wifi_sta_disconnected,
    .interface = NULL,
    .sock = -1,
};

static dmxbox_artnet_universe_advertisement_t *find_available_advertisement(
    dmxbox_artnet_universe_advertisement_t *head,
    uint8_t net,
    uint8_t subnet
) {
  for (dmxbox_artnet_universe_advertisement_t *packet = head; packet;
       packet = packet->next) {
    if (packet->net == net && packet->subnet == subnet &&
        packet->universe_count < MAX_UNIVERSES_IN_POLL_REPLY) {
      return packet;
    }
  }

  return NULL;
}

static void initialize_universes() {
  dmxbox_artnet_universe_t *universe1 = dmxbox_artnet_universe_alloc();
  universe1->address = 0;

  dmxbox_artnet_universe_t *universe2 = dmxbox_artnet_universe_alloc();
  universe2->address = 1;

  universe1->next = universe2;

  dmxbox_artnet_native_universe_address = universe1->address;
  universes_head = universe1;
  dmxbox_artnet_native_universe = universe1;
}

static void initialize_universe_advertisements() {
  dmxbox_artnet_universe_advertisement_t *head = NULL;
  dmxbox_artnet_universe_advertisement_t *tail = NULL;

  uint8_t bind_index = 1;

  for (dmxbox_artnet_universe_t *universe = universes_head; universe;
       universe = universe->next) {

    uint8_t net = (universe->address >> 8);
    uint8_t subnet = (universe->address >> 4) & 0xF;
    uint8_t universe_low = universe->address & 0xF;

    dmxbox_artnet_universe_advertisement_t *packet =
        find_available_advertisement(head, net, subnet);

    if (!packet) {
      packet = dmxbox_artnet_universe_advertisement_alloc();
      packet->bind_index = bind_index++;
      packet->net = net;
      packet->subnet = subnet;
      packet->universe_count = 0;

      if (tail) {
        tail->next = packet;
        tail = packet;
      } else {
        head = tail = packet;
      }
    }
    packet->universes_low[packet->universe_count] = universe_low;
    packet->universe_count++;
  }

  universe_advertisements_head = head;
}

void dmxbox_artnet_init() {
  ap_context.interface = wifi_get_ap_interface();
  sta_context.interface = wifi_get_sta_interface();

  dmxbox_artnet_client_tracking_init();

  initialize_universes();
  initialize_universe_advertisements();
}

int create_socket(esp_netif_t *interface, const char *context_name) {
  int sock = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
  if (sock < 0) {
    ESP_LOGE(TAG, "Unable to create %s socket: errno %d", context_name, errno);
    return sock;
  }

  struct ifreq ifr;
  ESP_ERROR_CHECK(esp_netif_get_netif_impl_name(interface, ifr.ifr_name));

  int ret = setsockopt(
      sock,
      SOL_SOCKET,
      SO_BINDTODEVICE,
      (void *)&ifr,
      sizeof(struct ifreq)
  );
  if (ret < 0) {
    ESP_LOGE(
        TAG,
        "Unable to bind %s socket to specified interface: errno %d",
        context_name,
        errno
    );
    lwip_close(sock);
    return ret;
  }

  return sock;
}

int create_and_bind_socket(esp_netif_t *interface, const char *context_name) {
  int sock = create_socket(interface, context_name);
  if (sock < 0) {
    ESP_LOGE(TAG, "Failed to create %s socket", context_name);
    return -1;
  }
  ESP_LOGI(TAG, "%s socket created", context_name);

  esp_netif_ip_info_t ip_info;
  ESP_ERROR_CHECK(esp_netif_get_ip_info(interface, &ip_info));

  struct sockaddr_in dest_addr;
  dest_addr.sin_addr.s_addr = ip_info.ip.addr;
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(ARTNET_PORT);

  ESP_LOGI(
      TAG,
      "binding to ip (%s): " IPSTR,
      context_name,
      IP2STR(&ip_info.ip)
  );

  int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
  if (err < 0) {
    ESP_LOGE(TAG, "%s socket unable to bind: errno %d", context_name, errno);
    shutdown(sock, 0);
    lwip_close(sock);
    return -1;
  }
  ESP_LOGI(TAG, "%s socket bound, port %d", context_name, ARTNET_PORT);

  return sock;
}

static void send_poll_reply_packet(
    int sock,
    esp_netif_t *interface,
    const dmxbox_artnet_universe_advertisement_t *packet_data
) {
  esp_netif_ip_info_t ip_info;
  ESP_ERROR_CHECK(esp_netif_get_ip_info(interface, &ip_info));

  struct artnet_reply_s reply = {0};
  memcpy(reply.id, PACKET_ID, sizeof(reply.id));
  reply.opCode = OP_POLL_REPLY;

  reply.ip[0] = reply.bindip[0] = ip_info.ip.addr & 0xFF;
  reply.ip[1] = reply.bindip[1] = (ip_info.ip.addr >> 8) & 0xFF;
  reply.ip[2] = reply.bindip[2] = (ip_info.ip.addr >> 16) & 0xFF;
  reply.ip[3] = reply.bindip[3] = (ip_info.ip.addr >> 24) & 0xFF;

  reply.port = ARTNET_PORT;

  reply.verH = (ARTNET_VERSION >> 8) & 0xFF;
  reply.ver = ARTNET_VERSION & 0xFF;
  reply.oemH = (OEM_UNKNOWN >> 8) & 0xFF;
  reply.oem = OEM_UNKNOWN & 0xFF;

  reply.status = (artnet_status_indicator_state_normal << 6) |
                 (artnet_status_programming_authority_unused << 4);
  reply.status2 =
      (1 << 3); // Node supports 15-bit Port-Address (Art-Net 3 or 4).

  const char *hostname = dmxbox_get_hostname();
  snprintf((char *)reply.shortname, sizeof(reply.shortname), "%s", hostname);
  snprintf((char *)reply.longname, sizeof(reply.longname), "%s", hostname);

  snprintf(
      (char *)reply.nodereport,
      sizeof(reply.nodereport),
      "#%04x [%04d] %s",
      artnet_node_report_power_ok,
      0,
      "OK"
  );

  reply.bindindex = packet_data->bind_index;

  reply.numbportsH = 0;
  reply.numbports = packet_data->universe_count;

  reply.subH = packet_data->net;
  reply.sub = packet_data->subnet;
  for (int i = 0; i < packet_data->universe_count; i++) {
    reply.swout[i] = packet_data->universes_low[i];
    reply.porttypes[i] = PORT_TYPE_OUTPUT;
  }

  ESP_LOGI(
      TAG,
      "Sending poll reply for universes %d-%d-x (%d universes)",
      reply.subH,
      reply.sub,
      packet_data->universe_count
  );

  reply.style = ST_NODE;

  struct sockaddr_in dest_addr;
  dest_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(ARTNET_PORT);

  int err = sendto(
      sock,
      (uint8_t *)&reply,
      sizeof(reply),
      0,
      (struct sockaddr *)&dest_addr,
      sizeof(dest_addr)
  );
  if (err < 0) {
    ESP_LOGE(TAG, "Error occurred during sending poll reply: errno %d", errno);
  }
}

static void send_poll_reply_to_socket(int sock, esp_netif_t *interface) {
  for (dmxbox_artnet_universe_advertisement_t *packet =
           universe_advertisements_head;
       packet;
       packet = packet->next) {
    send_poll_reply_packet(sock, interface, packet);
  }
}

static void send_periodic_poll_reply(dmxbox_artnet_listener_context_t *context
) {
  int sock = create_socket(context->interface, context->name);
  if (sock < 0) {
    ESP_LOGE(TAG, "Failed to create %s socket", context->name);
    return;
  }

  ESP_LOGI(TAG, "Sending periodic poll reply (%s)", context->name);
  send_poll_reply_to_socket(sock, context->interface);
  lwip_shutdown(sock, 0);
  lwip_close(sock);
}

static void handle_op_poll(
    int sock,
    esp_netif_t *interface,
    const struct sockaddr_storage *source_addr,
    const uint8_t *packet,
    int len
) {
  ESP_LOGI(TAG, "Received poll, sending poll reply");
  send_poll_reply_to_socket(sock, interface);
}

static void apply_changes(
    dmxbox_artnet_universe_t *universe,
    const uint8_t *last_data,
    const uint8_t *current_data,
    uint16_t data_length
) {
  taskENTER_CRITICAL(&dmxbox_artnet_spinlock);
  for (uint16_t i = 0; i < data_length; i++) {
    if (current_data[i] != last_data[i]) {
      universe->data[i] = current_data[i];
    }
  }
  taskEXIT_CRITICAL(&dmxbox_artnet_spinlock);

  if (LOG_DMX_DATA) {
    ESP_LOG_BUFFER_HEX(TAG, universe->data, 16);
  }
}

static dmxbox_artnet_universe_t *find_universe(uint16_t address) {
  for (dmxbox_artnet_universe_t *universe = universes_head; universe;
       universe = universe->next) {
    if (universe->address == address) {
      return universe;
    }
  }

  return NULL;
}

static void handle_dmx_data(
    dmxbox_artnet_universe_t *universe,
    const uint8_t *data,
    uint16_t data_length,
    const struct sockaddr_storage *source_addr,
    const char *addr_str
) {
  bool first_data_from_client = false;

  uint8_t client_universe_data[DMX_CHANNEL_COUNT];
  if (dmxbox_artnet_client_tracking_get_last_data(
          source_addr,
          universe->address,
          client_universe_data
      )) {
    apply_changes(universe, client_universe_data, data, data_length);
  } else {
    first_data_from_client = true;
  }

  dmxbox_artnet_client_tracking_set_last_data(
      source_addr,
      universe->address,
      data,
      data_length
  );

  if (first_data_from_client) {
    ESP_LOGI(
        TAG,
        "Received the first data from %s for universe %d",
        addr_str,
        universe->address
    );
  }
}

static void handle_op_dmx(
    int sock,
    const struct sockaddr_storage *source_addr,
    const char *addr_str,
    const uint8_t *packet,
    int len
) {
  if (len < 16) {
    ESP_LOGE(TAG, "Missing or incomplete universe address");
    return;
  }

  if (len < 16) {
    ESP_LOGE(TAG, "Missing or incomplete universe address");
    return;
  }
  uint16_t universe_address = packet[14] | packet[15] << 8;

  if (len < 18) {
    ESP_LOGE(TAG, "Missing or incomplete data length");
    return;
  }
  uint16_t data_length = packet[17] | packet[16] << 8;

  if (data_length == 0) {
    ESP_LOGE(TAG, "No data sent");
  }

  if (DMX_CHANNEL_COUNT < data_length) {
    ESP_LOGE(TAG, "Provided data length is too high: %d", data_length);
    return;
  }

  if ((len - 18) < data_length) {
    ESP_LOGE(
        TAG,
        "Data length mismatch: expected %d, got %d",
        data_length,
        len - 18
    );
    return;
  }

  if (LOG_DMX_DATA) {
    ESP_LOGI(
        TAG,
        "Received DMX data from %s for universe %d",
        addr_str,
        universe_address
    );
  }

  dmxbox_artnet_universe_t *universe = find_universe(universe_address);
  if (universe) {
    handle_dmx_data(universe, packet + 18, data_length, source_addr, addr_str);
  } else {
    if (LOG_DMX_DATA) {
      ESP_LOGI(
          TAG,
          "Received data with unknown universe address %d from %s",
          universe_address,
          addr_str
      );
    }
  }
}

static void handle_packet(
    int sock,
    esp_netif_t *interface,
    const struct sockaddr_storage *source_addr,
    const uint8_t *packet,
    int len
) {
  char addr_str[128];
  if (source_addr->ss_family == PF_INET) {
    inet_ntoa_r(
        ((struct sockaddr_in *)source_addr)->sin_addr,
        addr_str,
        sizeof(addr_str) - 1
    );
  } else if (source_addr->ss_family == PF_INET6) {
    inet6_ntoa_r(
        ((struct sockaddr_in6 *)source_addr)->sin6_addr,
        addr_str,
        sizeof(addr_str) - 1
    );
  }

  // ESP_LOGI(TAG, "Received %d bytes from %s", len, addr_str);

  if (len < 8) {
    ESP_LOGE(TAG, "Missing or incomplete packet header");
    return;
  }
  for (uint8_t i = 0; i < 8; i++) {
    if (packet[i] != PACKET_ID[i]) {
      ESP_LOGE(TAG, "Invalid packet header");
      return;
    }
  }

  if (len < 10) {
    ESP_LOGE(TAG, "Missing or incomplete opcode");
    return;
  }
  uint16_t opcode = packet[8] | packet[9] << 8;

  switch (opcode) {
  case OP_POLL:
    handle_op_poll(sock, interface, source_addr, packet, len);
    break;

  case OP_POLL_REPLY:
    break;

  case OP_DMX:
    handle_op_dmx(sock, source_addr, addr_str, packet, len);
    break;

  default:
    ESP_LOGI(TAG, "Received unsupported opcode: %d", opcode);
    break;
  }
}

static void reset_artnet_state() {
  taskENTER_CRITICAL(&dmxbox_artnet_spinlock);
  for (dmxbox_artnet_universe_t *universe = universes_head; universe;
       universe = universe->next) {
    memset(universe->data, 0, DMX_CHANNEL_COUNT);
  }
  taskEXIT_CRITICAL(&dmxbox_artnet_spinlock);

  dmxbox_artnet_client_tracking_reset();
}

static void reset_button_loop(void *parameter) {
  button_event_t ev;
  QueueHandle_t button_events =
      pulled_button_init(PIN_BIT(dmxbox_button_reset), GPIO_PULLUP_ONLY);

  while (1) {
    if (xQueueReceive(button_events, &ev, 1000 / portTICK_PERIOD_MS)) {
      if ((ev.pin == dmxbox_button_reset) && (ev.event == BUTTON_DOWN)) {
        ESP_LOGI(TAG, "Reset button pressed");

        taskENTER_CRITICAL(&dmxbox_artnet_spinlock);
        reset_artnet_state();
        taskEXIT_CRITICAL(&dmxbox_artnet_spinlock);
      }
    }
  }
}

void send_periodic_poll_reply_when_connected(void *parameter) {
  const EventBits_t events =
      dmxbox_wifi_ap_sta_connected | dmxbox_wifi_sta_connected;
  while (1) {
    EventBits_t bits = xEventGroupWaitBits(
        dmxbox_wifi_event_group,
        events,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    if (bits & dmxbox_wifi_ap_sta_connected) {
      send_periodic_poll_reply(&ap_context);
    }

    if (bits & dmxbox_wifi_sta_connected) {
      send_periodic_poll_reply(&sta_context);
    }

    vTaskDelay(POLL_REPLY_INTERVAL / portTICK_PERIOD_MS);
  }
}

void artnet_receive_loop(void *parameter) {
  dmxbox_artnet_listener_context_t *context =
      (dmxbox_artnet_listener_context_t *)parameter;

  int sock = context->sock;

  ESP_LOGI(TAG, "Listening for data (%s)", context->name);
  while (1) {
    struct sockaddr_storage source_addr;
    socklen_t socklen = sizeof(source_addr);
    int len = recvfrom(
        sock,
        context->packet_buffer,
        sizeof(context->packet_buffer),
        0,
        (struct sockaddr *)&source_addr,
        &socklen
    );

    // Error occurred during receiving
    if (len < 0) {
      if (errno == EBADF) {
        ESP_LOGI(TAG, "%s socket closed", context->name);
        break;
      }
      ESP_LOGE(TAG, "%s recvfrom failed: errno %d", context->name, errno);
      continue;
    }

    handle_packet(
        sock,
        context->interface,
        &source_addr,
        context->packet_buffer,
        len
    );
  }

  vTaskDelete(NULL);
}

void artnet_loop(void *parameter) {
  dmxbox_artnet_listener_context_t *context =
      (dmxbox_artnet_listener_context_t *)parameter;

  ESP_LOGI(TAG, "ArtNet %s receive task started", context->name);
  while (1) {
    xEventGroupWaitBits(
        dmxbox_wifi_event_group,
        context->connected_bit,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    ESP_LOGI(TAG, "Creating %s socket", context->name);
    context->sock = create_and_bind_socket(context->interface, context->name);
    if (context->sock != -1) {
      xTaskCreate(
          artnet_receive_loop,
          context->receive_task_name,
          10000,
          (void *)context,
          2,
          NULL
      );

      vTaskDelay(50 / portTICK_PERIOD_MS);

      xEventGroupWaitBits(
          dmxbox_wifi_event_group,
          context->disconnected_bit,
          pdFALSE,
          pdFALSE,
          portMAX_DELAY
      );

      ESP_LOGI(TAG, "Shutting down %s socket", context->name);
      shutdown(context->sock, 0);
      lwip_close(context->sock);
      context->sock = -1;
    }
  }
}

void dmxbox_artnet_receive_task(void *parameter) {
  xTaskCreate(reset_button_loop, "ArtNet reset", 4096, NULL, 1, NULL);

  xTaskCreate(
      send_periodic_poll_reply_when_connected,
      "ArtNet periodic poll reply",
      4096,
      NULL,
      1,
      NULL
  );

  xTaskCreate(
      artnet_loop,
      "ArtNet AP loop",
      10000,
      (void *)&ap_context,
      2,
      NULL
  );

  xTaskCreate(
      artnet_loop,
      "ArtNet STA loop",
      10000,
      (void *)&sta_context,
      2,
      NULL
  );

  vTaskDelete(NULL);
}

static bool artnet_active = false;

void dmxbox_set_artnet_active(bool state) {
  if (artnet_active != state) {
    ESP_ERROR_CHECK(dmxbox_led_set(dmxbox_led_artnet_in, state));
    artnet_active = state;
  }
}

const uint8_t *dmxbox_artnet_get_native_universe_data() {
  return dmxbox_artnet_native_universe->data;
}

const uint8_t *dmxbox_artnet_get_universe_data(uint16_t address) {
  const dmxbox_artnet_universe_t *universe = find_universe(address);
  return universe ? universe->data : NULL;
}
