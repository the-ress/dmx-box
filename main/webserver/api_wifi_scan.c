#include "api_wifi_scan.h"
#include "wifi_scan.h"
#include <esp_check.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <stddef.h>
#include <stdint.h>
#include <strings.h>

static const char TAG[] = "dmxbox_api_wifi_scan";

#ifndef CONFIG_DMXBOX_MAX_WEBSOCKETS
#define CONFIG_DMXBOX_MAX_WEBSOCKETS 2
#endif

static int websocket_fds[CONFIG_DMXBOX_MAX_WEBSOCKETS];
static int websocket_fd_count = 0;
static httpd_handle_t wifi_scan_httpd_server = NULL;

static void dmxbox_api_wifi_scan_erase_ws(int ws) {
  for (; ws < websocket_fd_count - 1; ws++) {
    websocket_fds[ws] = websocket_fds[ws + 1];
  }
  websocket_fd_count--;
}

// only called on the httpd task
static esp_err_t dmxbox_api_wifi_scan_push_ws(httpd_req_t *req) {
  if (websocket_fd_count == CONFIG_DMXBOX_MAX_WEBSOCKETS) {
    return ESP_ERR_NO_MEM;
  }
  websocket_fds[websocket_fd_count] = httpd_req_to_sockfd(req);
  websocket_fd_count++;
  return ESP_OK;
}

// runs on the httpd task
static void dmxbox_api_wifi_scan_send_records(void *arg) {
  dmxbox_wifi_scan_result_t *result = arg;
  for (int ws = 0; ws < websocket_fd_count; ws++) {
    for (int r = 0; r < result->count; r++) {
      httpd_ws_frame_t ws_frame = {
          .payload = result->records[r].ssid,
          .len = strlen((const char *)result->records[r].ssid),
          .type = HTTPD_WS_TYPE_TEXT
      };

      ESP_LOGI(TAG, "sending record %d to websocket %d", r, ws);
      esp_err_t send_err = httpd_ws_send_frame_async(
          wifi_scan_httpd_server,
          websocket_fds[ws],
          &ws_frame
      );

      if (send_err != ESP_OK) {
        ESP_LOGI(TAG, "failed to send to websocket %d, removing it", ws);
        dmxbox_api_wifi_scan_erase_ws(ws);
        break;
      }
    }
  }
  free(result);
}

// runs on the wifi event task
static void dmxbox_api_wifi_scan_callback(dmxbox_wifi_scan_result_t *result) {
  ESP_LOGI(
      TAG,
      "wifi_scan callback, got %d records, queueing http work",
      result->count
  );
  esp_err_t ret = ESP_OK;
  ESP_GOTO_ON_ERROR(
      httpd_queue_work(
          wifi_scan_httpd_server,
          dmxbox_api_wifi_scan_send_records,
          result
      ),
      error,
      TAG,
      "failed to queue work"
  );
error:
  (void)ret;
}

static esp_err_t dmxbox_api_wifi_scan_req_handler(httpd_req_t *req) {
  if (req->method == HTTP_GET) {
    ESP_LOGI(TAG, "connected");
    ESP_RETURN_ON_ERROR(
        dmxbox_api_wifi_scan_push_ws(req),
        TAG,
        "failed to push ws"
    );
    ESP_RETURN_ON_ERROR(dmxbox_wifi_scan_start(), TAG, "failed to start scan");
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

esp_err_t dmxbox_api_wifi_scan_register(httpd_handle_t server) {

  ESP_LOGI(TAG, "registering wifi_scan endpoint");

  static const httpd_uri_t uri = {
      .uri = "/api/wifi-scan",
      .method = HTTP_GET,
      .handler = dmxbox_api_wifi_scan_req_handler,
      .is_websocket = true,
  };

  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &uri),
      TAG,
      "handler_wifi_scan failed"
  );

  ESP_RETURN_ON_ERROR(
      dmxbox_wifi_scan_register_callback(dmxbox_api_wifi_scan_callback),
      TAG,
      "failed to register wifi_scan callback"
  );

  wifi_scan_httpd_server = server;
  return ESP_OK;
}
