#include "wifi_scan.h"
#include "wifi.h"
#include <esp_check.h>
#include <stdint.h>
#include <strings.h>

static const char TAG[] = "dmxbox_api_wifi_scan";

static void dmxbox_httpd_wifi_scan_callback(
    uint16_t record_count,
    const wifi_ap_record_t *records
) {
  for (uint16_t i = 0; i < record_count; i++) {
    ESP_LOGI(TAG, "found AP %s", records[i].ssid);
  }
}

esp_err_t dmxbox_httpd_wifi_scan_handler(httpd_req_t *req) {
  if (req->method == HTTP_GET) {
    ESP_LOGI(TAG, "connected");
    return dmxbox_wifi_start_scan(&dmxbox_httpd_wifi_scan_callback);
  }

  static const char buffer[] = "Hello, WebSocket";

  httpd_ws_frame_t ws_frame = {
      .type = HTTPD_WS_TYPE_TEXT,
      .payload = (uint8_t *)buffer,
      .len = sizeof(buffer) - 1,
  };

  ESP_RETURN_ON_ERROR(
      httpd_ws_send_frame(req, &ws_frame),
      TAG,
      "failed to send ws frame"
  );

  return ESP_OK;
}
