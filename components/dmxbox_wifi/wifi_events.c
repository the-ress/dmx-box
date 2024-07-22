#include <esp_check.h>
#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#include "dmxbox_led.h"
#include "dmxbox_storage.h"
#include "private.h"
#include "wifi.h"

static const char *TAG = "dmxbox_wifi_events";

EventGroupHandle_t dmxbox_wifi_event_group;
static esp_timer_handle_t sta_retry_backoff_timer;

static bool is_in_disconnect = false;
static int retry_num = 0;

esp_err_t dmxbox_wifi_connect() {
  retry_num = 0;
  esp_timer_stop(sta_retry_backoff_timer);

  return esp_wifi_connect();
}

esp_err_t dmxbox_wifi_disconnect() {
  is_in_disconnect = true;
  esp_err_t ret = esp_wifi_disconnect();
  is_in_disconnect = false;
  return ret;
}

static void dmxbox_wifi_on_ap_start() {
  ESP_LOGI(TAG, "AP started");
  xEventGroupClearBits(dmxbox_wifi_event_group, dmxbox_wifi_ap_stopped);
  xEventGroupSetBits(dmxbox_wifi_event_group, dmxbox_wifi_ap_started);
}

static void dmxbox_wifi_on_ap_stop() {
  ESP_LOGI(TAG, "AP stopped");
  xEventGroupClearBits(dmxbox_wifi_event_group, dmxbox_wifi_ap_started);
  xEventGroupSetBits(dmxbox_wifi_event_group, dmxbox_wifi_ap_stopped);
}

static void
dmxbox_wifi_on_ap_staconnected(const wifi_event_ap_staconnected_t *event) {
  ESP_LOGI(
      TAG,
      "station " MACSTR " join, AID=%d",
      MAC2STR(event->mac),
      event->aid
  );

  dmxbox_led_set(dmxbox_led_ap, 1);

  xEventGroupSetBits(dmxbox_wifi_event_group, dmxbox_wifi_ap_sta_connected);
}

static void
dmxbox_wifi_on_ap_stadisconnected(wifi_event_ap_stadisconnected_t *event) {
  ESP_LOGI(
      TAG,
      "station " MACSTR " leave, AID=%d",
      MAC2STR(event->mac),
      event->aid
  );

  wifi_sta_list_t sta_list;
  esp_wifi_ap_get_sta_list(&sta_list);

  ESP_LOGI(TAG, "%d stations remain connected ", sta_list.num);
  dmxbox_led_set(dmxbox_led_ap, sta_list.num != 0);
}

static void sta_retry_backoff_callback(void *arg) {
  if (!dmxbox_get_sta_mode_enabled()) {
    ESP_LOGI(TAG, "STA not enabled, not reconnecting");
    return;
  }

  ESP_LOGI(TAG, "retrying to connect as STA after a backoff");
  dmxbox_wifi_connect();
}

static void dmxbox_wifi_on_sta_disconnected() {
  ESP_LOGI(TAG, "STA disconnected");

  dmxbox_led_set(dmxbox_led_sta, 0);
  xEventGroupClearBits(dmxbox_wifi_event_group, dmxbox_wifi_sta_connected);
  xEventGroupSetBits(dmxbox_wifi_event_group, dmxbox_wifi_sta_disconnected);

  if (is_in_disconnect) {
    ESP_LOGI(TAG, "is_in_disconnect true, not reconnecting as STA");
    return;
  }

  if (!dmxbox_get_sta_mode_enabled()) {
    ESP_LOGI(TAG, "STA not enabled, not reconnecting");
    return;
  }

  if (retry_num < CONFIG_WIFI_STA_MAXIMUM_RETRY) {
    ESP_LOGI(TAG, "retrying to connect as STA");
    retry_num++;
    esp_timer_stop(sta_retry_backoff_timer);

    esp_wifi_connect();
    return;
  }

  ESP_LOGE(
      TAG,
      "ran out of STA retry attempts, waiting %d ms",
      CONFIG_WIFI_STA_RETRY_BACKOFF_MS
  );
  xEventGroupSetBits(dmxbox_wifi_event_group, dmxbox_wifi_sta_fail);
  ESP_ERROR_CHECK(esp_timer_start_once(
      sta_retry_backoff_timer,
      CONFIG_WIFI_STA_RETRY_BACKOFF_MS * 1000
  ));
}

static void dmxbox_wifi_on_sta_got_ip(const ip_event_got_ip_t *event) {
  dmxbox_led_set(dmxbox_led_sta, 1);

  ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
  retry_num = 0;
  esp_timer_stop(sta_retry_backoff_timer);

  xEventGroupClearBits(dmxbox_wifi_event_group, dmxbox_wifi_sta_disconnected);
  xEventGroupSetBits(dmxbox_wifi_event_group, dmxbox_wifi_sta_connected);
}

static void dmxbox_wifi_on_wifi_event(
    void *arg,                   // NULL
    esp_event_base_t event_base, // WIFI_EVENT
    int32_t event_id,
    void *event_data
) {
  switch (event_id) {
  case WIFI_EVENT_AP_START:
    dmxbox_wifi_on_ap_start();
    break;

  case WIFI_EVENT_AP_STOP:
    dmxbox_wifi_on_ap_stop();
    break;

  case WIFI_EVENT_AP_STACONNECTED:
    dmxbox_wifi_on_ap_staconnected(event_data);
    break;

  case WIFI_EVENT_AP_STADISCONNECTED:
    dmxbox_wifi_on_ap_stadisconnected(event_data);
    break;

  case WIFI_EVENT_STA_START:
    dmxbox_wifi_connect();
    break;

  case WIFI_EVENT_STA_DISCONNECTED:
    dmxbox_wifi_on_sta_disconnected();
    break;

  case WIFI_EVENT_SCAN_DONE:
    dmxbox_wifi_scan_on_done();
    break;

  default:
    break;
  }
}

static void dmxbox_wifi_on_ip_event(
    void *arg,                   // NULL
    esp_event_base_t event_base, // IP_EVENT
    int32_t event_id,
    void *event_data
) {
  switch (event_id) {
  case IP_EVENT_STA_GOT_IP:
    dmxbox_wifi_on_sta_got_ip(event_data);
    break;

  default:
    break;
  }
}

static void create_sta_retry_backoff_timer() {
  const esp_timer_create_args_t args = {
      .callback = &sta_retry_backoff_callback,
      .name = "sta_retry_backoff_timer",
  };

  ESP_ERROR_CHECK(esp_timer_create(&args, &sta_retry_backoff_timer));
}

void dmxbox_wifi_register_event_handlers() {
  dmxbox_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_event_handler_register(
      WIFI_EVENT,
      ESP_EVENT_ANY_ID,
      &dmxbox_wifi_on_wifi_event,
      NULL
  ));

  ESP_ERROR_CHECK(esp_event_handler_register(
      IP_EVENT,
      IP_EVENT_STA_GOT_IP,
      &dmxbox_wifi_on_ip_event,
      NULL
  ));

  create_sta_retry_backoff_timer();
}