#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <sdkconfig.h>

#include "button.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/sockets.h"

#include "artnet.h"
#include "artnet_const.h"
#include "const.h"
#include "hashmap.h"
#include "dmxbox_led.h"
#include "wifi.h"
#include <stdint.h>

static const char *TAG = "artnet";

// Spec: http://www.artisticlicence.com/WebSiteMaster/User%20Guides/art-net.pdf

#define SHORT_NAME "DMX Box"
#define LONG_NAME "Art-Net -> DMX converter"
#define LOG_DMX_DATA false
static const uint32_t POLL_REPLY_INTERVAL = 10 * 1000;

struct map_entry {
  struct sockaddr_storage addr;
  uint8_t *last_data;
};

struct listener_context {
  char *name;
  char *receive_task_name;
  char *poll_reply_task_name;
  EventBits_t connected_bit;
  EventBits_t disconnected_bit;
  esp_netif_t *interface;
  int sock;
  uint8_t packet_buffer[MAX_PACKET_SIZE];
};

portMUX_TYPE dmxbox_artnet_spinlock = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE client_map_spinlock = portMUX_INITIALIZER_UNLOCKED;
uint8_t dmxbox_artnet_in_data[DMX_CHANNEL_COUNT] = {0};
static struct hashmap *client_map;
static bool connected = false;

struct listener_context ap_context = {
    .name = "AP",
    .receive_task_name = "ArtNet AP receive loop",
    .poll_reply_task_name = "ArtNet AP poll reply",
    .connected_bit = dmxbox_wifi_ap_sta_connected,
    .disconnected_bit = dmxbox_wifi_ap_stopped,
    .interface = NULL,
    .sock = -1,
};

struct listener_context sta_context = {
    .name = "STA",
    .receive_task_name = "ArtNet STA receive loop",
    .poll_reply_task_name = "ArtNet STA poll reply",
    .connected_bit = dmxbox_wifi_sta_connected,
    .disconnected_bit = dmxbox_wifi_sta_disconnected,
    .interface = NULL,
    .sock = -1,
};

void dmxbox_artnet_initialize(void) {
  ap_context.interface = wifi_get_ap_interface();
  sta_context.interface = wifi_get_sta_interface();
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

static int ip_compare(const void *a, const void *b, void *udata) {
  const struct map_entry *entrya = a;
  const struct map_entry *entryb = b;

  if (entrya->addr.ss_family != entryb->addr.ss_family) {
    return entrya->addr.ss_family - entryb->addr.ss_family;
  }

  switch (entrya->addr.ss_family) {
  case PF_INET: {
    struct in_addr ipa = ((struct sockaddr_in *)&entrya->addr)->sin_addr;
    struct in_addr ipb = ((struct sockaddr_in *)&entryb->addr)->sin_addr;
    return memcmp(&ipa, &ipb, sizeof(ipa));
  }
  case PF_INET6: {
    struct in6_addr ipa = ((struct sockaddr_in6 *)&entrya->addr)->sin6_addr;
    struct in6_addr ipb = ((struct sockaddr_in6 *)&entryb->addr)->sin6_addr;
    return memcmp(&ipa, &ipb, sizeof(ipa));
  }
  }

  ESP_LOGE(TAG, "Unexpected address family: %d", entrya->addr.ss_family);
  return 0;
}

static uint64_t ip_hash(const void *item, uint64_t seed0, uint64_t seed1) {
  const struct map_entry *entry = item;

  switch (entry->addr.ss_family) {
  case PF_INET: {
    struct in_addr ip = ((struct sockaddr_in *)&entry->addr)->sin_addr;
    return hashmap_sip(&ip, sizeof(ip), seed0, seed1);
  }
  case PF_INET6: {
    struct in6_addr ip = ((struct sockaddr_in6 *)&entry->addr)->sin6_addr;
    return hashmap_sip(&ip, sizeof(ip), seed0, seed1);
  }
  }

  ESP_LOGE(TAG, "Unexpected address family: %d", entry->addr.ss_family);
  return 0;
}

static void free_map_entry(void *item) {
  struct map_entry *entry = item;
  if (entry->last_data) {
    free(entry->last_data);
    entry->last_data = NULL;
  }
}

static void send_poll_reply_to_socket(int sock, esp_netif_t *interface) {
  esp_netif_ip_info_t ip_info;
  ESP_ERROR_CHECK(esp_netif_get_ip_info(interface, &ip_info));

  struct artnet_reply_s reply = {0};
  memcpy(reply.id, PACKET_ID, sizeof(reply.id));
  reply.opCode = OP_POLL_REPLY;

  reply.ip[0] = ip_info.ip.addr & 0xFF;
  reply.ip[1] = (ip_info.ip.addr >> 8) & 0xFF;
  reply.ip[2] = (ip_info.ip.addr >> 16) & 0xFF;
  reply.ip[3] = (ip_info.ip.addr >> 24) & 0xFF;

  reply.port = ARTNET_PORT;

  reply.verH = (ARTNET_VERSION >> 8) & 0xFF;
  reply.ver = ARTNET_VERSION & 0xFF;
  reply.oemH = (OEM_UNKNOWN >> 8) & 0xFF;
  reply.oem = OEM_UNKNOWN & 0xFF;

  sprintf((char *)reply.shortname, SHORT_NAME);
  sprintf((char *)reply.longname, LONG_NAME);
  sprintf((char *)reply.nodereport, "Active");

  reply.numbportsH = 0;
  reply.numbports = 1;
  reply.porttypes[0] = PORT_TYPE_OUTPUT;
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

static void send_periodic_poll_reply(struct listener_context *context) {
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
    const uint8_t *last_data,
    const uint8_t *current_data,
    uint16_t data_length
) {
  taskENTER_CRITICAL(&dmxbox_artnet_spinlock);
  for (uint16_t i = 0; i < data_length; i++) {
    if (current_data[i] != last_data[i]) {
      dmxbox_artnet_in_data[i] = current_data[i];
    }
  }
  taskEXIT_CRITICAL(&dmxbox_artnet_spinlock);

  if (LOG_DMX_DATA) {
    ESP_LOG_BUFFER_HEX(TAG, dmxbox_artnet_in_data, 16);
  }
}

static void handle_dmx_data(
    const uint8_t *data,
    uint16_t data_length,
    const struct sockaddr_storage *source_addr,
    const char *addr_str
) {
  uint8_t *last_data;
  struct map_entry entry = {.addr = *source_addr};
  bool first_data_from_client = false;

  taskENTER_CRITICAL(&client_map_spinlock);
  struct map_entry *last_entry = hashmap_get(client_map, &entry);
  if (last_entry) {
    last_data = last_entry->last_data;
    apply_changes(last_data, data, data_length);
  } else {
    last_data = calloc(DMX_CHANNEL_COUNT, sizeof(*last_data));
    entry.last_data = last_data;
    hashmap_set(client_map, &entry);
    first_data_from_client = true;
  }

  if (data_length != DMX_CHANNEL_COUNT) {
    memset(last_data, 0, DMX_CHANNEL_COUNT);
  }
  memcpy(last_data, data, data_length);
  taskEXIT_CRITICAL(&client_map_spinlock);

  if (first_data_from_client) {
    ESP_LOGI(TAG, "Received the first data from %s", addr_str);
  }
}

static void handle_op_dmx(
    int sock,
    const struct sockaddr_storage *source_addr,
    const char *addr_str,
    const uint8_t *packet,
    int len
) {
  if (LOG_DMX_DATA) {
    ESP_LOGI(TAG, "Received DMX data from %s", addr_str);
  }

  if (len < 16) {
    ESP_LOGE(TAG, "Missing or incomplete universe");
    return;
  }

  if (len < 16) {
    ESP_LOGE(TAG, "Missing or incomplete universe");
    return;
  }
  uint16_t universe = packet[14] | packet[15] << 8;

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

  if (universe == 0) {
    if (!connected) {
      connected = true;
      ESP_LOGI(TAG, "ArtNet connected");
    }

    handle_dmx_data(packet + 18, data_length, source_addr, addr_str);
  } else {
    if (LOG_DMX_DATA) {
      ESP_LOGI(
          TAG,
          "Received data for unknown universe %d from %s",
          universe,
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

static void reset_artnet_state(void) {
  taskENTER_CRITICAL(&dmxbox_artnet_spinlock);
  memset(dmxbox_artnet_in_data, 0, DMX_CHANNEL_COUNT);
  taskEXIT_CRITICAL(&dmxbox_artnet_spinlock);

  taskENTER_CRITICAL(&client_map_spinlock);
  hashmap_clear(client_map, true);
  taskEXIT_CRITICAL(&client_map_spinlock);
}

static void reset_button_loop(void *parameter) {
  button_event_t ev;
  QueueHandle_t button_events =
      pulled_button_init(PIN_BIT(RESET_BUTTON_GPIO), GPIO_PULLUP_ONLY);

  while (1) {
    if (xQueueReceive(button_events, &ev, 1000 / portTICK_PERIOD_MS)) {
      if ((ev.pin == RESET_BUTTON_GPIO) && (ev.event == BUTTON_DOWN)) {
        ESP_LOGI(TAG, "Reset button pressed");

        taskENTER_CRITICAL(&dmxbox_artnet_spinlock);
        reset_artnet_state();
        taskEXIT_CRITICAL(&dmxbox_artnet_spinlock);
      }
    }
  }
}

void send_poll_reply_on_connect(void *parameter) {
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
  struct listener_context *context = (struct listener_context *)parameter;

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
  struct listener_context *context = (struct listener_context *)parameter;

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
  client_map = hashmap_new(
      sizeof(struct map_entry),
      0,
      0,
      0,
      ip_hash,
      ip_compare,
      free_map_entry,
      NULL
  );

  xTaskCreate(reset_button_loop, "ArtNet reset", 4096, NULL, 1, NULL);

  xTaskCreate(
      send_poll_reply_on_connect,
      "ArtNet poll reply",
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
