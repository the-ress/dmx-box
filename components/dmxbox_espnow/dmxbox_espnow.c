#include <esp_crc.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_now.h>
#include <string.h>

#include "dmxbox_espnow.h"
#include "esp_random.h"

static const char *TAG = "dmxbox_espnow";

#define CONFIG_ESPNOW_PMK "pmk1234567890123"
#define CONFIG_ESPNOW_LMK "lmk1234567890123"
#define CONFIG_ESPNOW_SEND_COUNT 100
#define CONFIG_ESPNOW_SEND_DELAY 1000

#define ESPNOW_QUEUE_SIZE 6

#define ESPNOW_MAXDELAY 512

typedef struct {
  uint8_t mac_addr[ESP_NOW_ETH_ALEN];
  esp_now_send_status_t status;
} send_queue_event_t;

typedef struct {
  uint8_t mac_addr[ESP_NOW_ETH_ALEN];
  uint8_t *data;
  int data_len;
} recv_queue_event_t;

/* User defined field of ESPNOW data in this example. */
typedef struct __attribute__((packed)) {
  uint16_t effect_id;
  uint8_t level;
  uint8_t rate_raw;
  double progress;
  bool first_pass;
  uint16_t crc;
} effect_sync_packet_t;

static dmxbox_espnow_effect_state_callback_t effect_state_callback = NULL;

static QueueHandle_t send_queue;
static QueueHandle_t recv_queue;

static const uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] =
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void
dmxbox_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (mac_addr == NULL) {
    ESP_LOGE(TAG, "Send cb arg error");
    return;
  }

  send_queue_event_t evt = {
      .status = status,
  };
  memcpy(evt.mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
  if (xQueueSend(send_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
    ESP_LOGW(TAG, "Send queue fail");
  }
}

static void dmxbox_espnow_recv_cb(
    const esp_now_recv_info_t *recv_info,
    const uint8_t *data,
    int len
) {
  uint8_t *mac_addr = recv_info->src_addr;

  if (mac_addr == NULL || data == NULL || len <= 0) {
    ESP_LOGE(TAG, "Receive cb arg error");
    return;
  }

  recv_queue_event_t evt = {
      .data = malloc(len),
  };
  if (evt.data == NULL) {
    ESP_LOGE(TAG, "Malloc receive data fail");
    return;
  }
  memcpy(evt.mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
  memcpy(evt.data, data, len);
  evt.data_len = len;
  if (xQueueSend(recv_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
    ESP_LOGW(TAG, "Receive queue fail");
    free(evt.data);
  }
}

/* Parse received ESPNOW data. */
bool process_incoming_effect_sync_packet(
    uint8_t *data,
    uint16_t data_len,
    effect_sync_packet_t *result_packet
) {
  if (data_len < sizeof(effect_sync_packet_t)) {
    ESP_LOGE(TAG, "Receive ESPNOW data too short, len: %d", data_len);
    return false;
  }

  effect_sync_packet_t *packet = (effect_sync_packet_t *)data;

  uint16_t crc = packet->crc;
  packet->crc = 0;
  uint16_t crc_cal =
      esp_crc16_le(UINT16_MAX, (uint8_t const *)packet, data_len);

  if (crc_cal != crc) {
    ESP_LOGE(TAG, "ESPNOW data CRC doesn't match");
    return false;
  }

  memcpy(result_packet, packet, sizeof(effect_sync_packet_t));
  return true;
}

void fill_effect_sync_packet_crc(effect_sync_packet_t *packet) {
  packet->crc = 0;
  packet->crc = esp_crc16_le(
      UINT16_MAX,
      (uint8_t const *)packet,
      sizeof(effect_sync_packet_t)
  );
}

static void espnow_send_loop(void *pvParameter) {
  // while (!sending) {
  //   vTaskDelay(100 / portTICK_PERIOD_MS);
  // }

  // ESP_LOGI(TAG, "Start sending broadcast data");

  /* Start sending broadcast ESPNOW data. */
  // fill_packet_buffer();
  // ESP_ERROR_CHECK(esp_now_send(broadcast_mac, packet_buffer,
  // sizeof(packet_t)));

  send_queue_event_t evt;
  while (xQueueReceive(send_queue, &evt, portMAX_DELAY) == pdTRUE) {
    ESP_LOGI(
        TAG,
        "Send data to " MACSTR ", status: %d",
        MAC2STR(evt.mac_addr),
        evt.status
    );

    // /* Delay a while before sending the next data. */
    // if (delay > 0) {
    //   vTaskDelay(delay / portTICK_PERIOD_MS);
    // }

    // ESP_LOGI(TAG, "send data to " MACSTR "", MAC2STR(evt.mac_addr));

    // fill_packet_buffer();
    // ESP_ERROR_CHECK(esp_now_send(broadcast_mac, packet_buffer,
    // sizeof(packet_t))
    // );
  }
}

static void espnow_recv_loop(void *pvParameter) {
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  ESP_LOGI(TAG, "Start receiving broadcast data");

  recv_queue_event_t evt;
  while (xQueueReceive(recv_queue, &evt, portMAX_DELAY) == pdTRUE) {
    effect_sync_packet_t packet;
    bool success =
        process_incoming_effect_sync_packet(evt.data, evt.data_len, &packet);
    free(evt.data);

    if (success) {
      ESP_LOGI(
          TAG,
          "Received effect state data from: " MACSTR
          ", len: %d, level: %d, rate: %d, progress: %f, first_pass: %d",
          MAC2STR(evt.mac_addr),
          evt.data_len,
          packet.level,
          packet.rate_raw,
          packet.progress,
          packet.first_pass
      );
      if (effect_state_callback) {
        effect_state_callback(
            packet.effect_id,
            packet.level,
            packet.rate_raw,
            packet.progress,
            packet.first_pass
        );
      }
    } else {
      ESP_LOGI(
          TAG,
          "Received invalid data from: " MACSTR "",
          MAC2STR(evt.mac_addr)
      );
    }

    /* If MAC address does not exist in peer list, add it to peer list. */
    if (esp_now_is_peer_exist(evt.mac_addr) == false) {
      esp_now_peer_info_t *peer = calloc(1, sizeof(esp_now_peer_info_t));
      peer->channel = 0; // use current
      peer->ifidx = ESP_IF_WIFI_AP;
      peer->encrypt = true;
      memcpy(peer->lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
      memcpy(peer->peer_addr, evt.mac_addr, ESP_NOW_ETH_ALEN);
      ESP_ERROR_CHECK(esp_now_add_peer(peer));
      free(peer);
    }
  }
}

void dmxbox_espnow_register_effect_state_callback(
    dmxbox_espnow_effect_state_callback_t cb
) {
  if (effect_state_callback) {
    ESP_LOGW(TAG, "Effect state callback is already set");
  }

  effect_state_callback = cb;
}

void dmxbox_espnow_send_effect_state(
    uint16_t effect_id,
    uint8_t level,
    uint8_t rate_raw,
    double progress,
    bool first_pass
) {
  effect_sync_packet_t packet = {
      .effect_id = effect_id,
      .level = level,
      .rate_raw = rate_raw,
      .progress = progress,
      .first_pass = first_pass,
  };

  fill_effect_sync_packet_crc(&packet);
  ESP_ERROR_CHECK(esp_now_send(
      broadcast_mac,
      (uint8_t *)&packet,
      sizeof(effect_sync_packet_t)
  ));
}

void dmxbox_espnow_init() {
  send_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(send_queue_event_t));
  recv_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(recv_queue_event_t));

  ESP_ERROR_CHECK(esp_now_init());
  ESP_ERROR_CHECK(esp_now_register_send_cb(dmxbox_espnow_send_cb));
  ESP_ERROR_CHECK(esp_now_register_recv_cb(dmxbox_espnow_recv_cb));

  ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK));

  // Add broadcast peer information to peer list.
  esp_now_peer_info_t *peer = calloc(1, sizeof(esp_now_peer_info_t));
  peer->channel = 0; // use current
  peer->ifidx = ESP_IF_WIFI_AP;
  peer->encrypt = false;
  memcpy(peer->peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
  ESP_ERROR_CHECK(esp_now_add_peer(peer));
  free(peer);

  xTaskCreate(espnow_send_loop, "espnow_send_loop", 2048, NULL, 4, NULL);
  xTaskCreate(espnow_recv_loop, "espnow_recv_loop", 2048, NULL, 4, NULL);
}

// static void example_espnow_deinit() {
//   vSemaphoreDelete(send_queue);
//   vSemaphoreDelete(recv_queue);
//   esp_now_deinit();
// }
