
#include "wifi_scan.h"
#include <esp_check.h>
#include <esp_event.h>
#include <esp_log.h>
#include <stddef.h>

static const char TAG[] = "dmxbox_wifi_scan";

static dmxbox_wifi_scan_callback_t g_callback = NULL;

void dmxbox_wifi_scan_on_done() {
  esp_err_t ret = ESP_OK;
  uint16_t record_count = 0;
  dmxbox_wifi_scan_result_t *result = NULL;

  ESP_GOTO_ON_ERROR(
      esp_wifi_scan_get_ap_num(&record_count),
      failed,
      TAG,
      "scan_get_ap_num failed"
  );

  ESP_LOGI(TAG, "received %d AP records", record_count);

  result = malloc(
      sizeof(dmxbox_wifi_scan_result_t) +
      (sizeof(wifi_ap_record_t) * (record_count - 1))
  );

  if (!result) {
    ESP_LOGE(TAG, "failed to malloc AP records");
    return;
  }

  result->count = record_count;

  ESP_GOTO_ON_ERROR(
      esp_wifi_scan_get_ap_records(&result->count, result->records),
      failed,
      TAG,
      "scan_get_ap_records failed"
  );

  if (g_callback) {
    g_callback(result);
    return;
  }

failed:
  if (result) {
    free(result);
  }
  (void)ret;
}

esp_err_t
dmxbox_wifi_scan_register_callback(dmxbox_wifi_scan_callback_t callback) {
  g_callback = callback;
  return ESP_OK;
}

esp_err_t dmxbox_wifi_scan_start() {
  ESP_RETURN_ON_ERROR(esp_wifi_scan_stop(), TAG, "failed to stop scan");
  ESP_RETURN_ON_ERROR(
      esp_wifi_scan_start(NULL, false),
      TAG,
      "failed to start scan"
  );
  return ESP_OK;
}
