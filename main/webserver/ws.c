#include "ws.h"
#include "webserver.h"
#include "wifi_scan.h"
#include <esp_check.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <strings.h>

static const char TAG[] = "dmxbox_ws";
static httpd_handle_t dmxbox_ws_server;

// runs on the httpd task
static void dmxbox_ws_send_ap_records(void *arg) {
  dmxbox_wifi_scan_result_t *result = arg;
  esp_err_t ret = ESP_OK;

  int client_fds[CONFIG_DMXBOX_WEBSEVER_MAX_SOCKETS];
  size_t clients = CONFIG_DMXBOX_WEBSEVER_MAX_SOCKETS;
  ESP_GOTO_ON_ERROR(
      httpd_get_client_list(dmxbox_ws_server, &clients, client_fds),
      exit,
      TAG,
      "failed to get client list"
  );

  for (int c = 0; c < clients; c++) {
    httpd_ws_client_info_t client_info =
        httpd_ws_get_fd_info(dmxbox_ws_server, client_fds[c]);
    if (client_info != HTTPD_WS_CLIENT_WEBSOCKET) {
      continue;
    }

    for (int r = 0; r < result->count; r++) {
      httpd_ws_frame_t ws_frame = {
          .payload = result->records[r].ssid,
          .len = strlen((const char *)result->records[r].ssid),
          .type = HTTPD_WS_TYPE_TEXT
      };

      ESP_LOGI(TAG, "sending record %d to websocket %d", r, c);
      httpd_ws_send_frame_async(dmxbox_ws_server, client_fds[c], &ws_frame);
    }
  }

exit:
  free(result);
  (void)ret;
}

// runs on the wifi event task
static void dmxbox_ws_wifi_scan_callback(dmxbox_wifi_scan_result_t *result) {
  ESP_LOGI(
      TAG,
      "ws_wifi_scan callback, got %d records, queueing http work",
      result->count
  );
  esp_err_t ret = ESP_OK;
  ESP_GOTO_ON_ERROR( // TODO replace with ESP_RETURN_VOID_ON_ERROR
      httpd_queue_work(dmxbox_ws_server, dmxbox_ws_send_ap_records, result),
      error,
      TAG,
      "failed to queue work"
  );
error:
  (void)ret;
}

static esp_err_t dmxbox_ws_handler(httpd_req_t *req) {
  if (req->method == HTTP_GET) {
    ESP_LOGI(TAG, "connected");
    ESP_RETURN_ON_ERROR(dmxbox_wifi_scan_start(), TAG, "failed to start scan");
  }
  return ESP_OK;
}

esp_err_t dmxbox_ws_register(httpd_handle_t server) {

  ESP_LOGI(TAG, "registering wifi_scan endpoint");

  static const httpd_uri_t uri = {
      .uri = "/api/ws",
      .method = HTTP_GET,
      .handler = dmxbox_ws_handler,
      .is_websocket = true,
  };

  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &uri),
      TAG,
      "failed to register ws handler"
  );

  ESP_RETURN_ON_ERROR(
      dmxbox_wifi_scan_register_callback(dmxbox_ws_wifi_scan_callback),
      TAG,
      "failed to register wifi_scan callback"
  );

  dmxbox_ws_server = server;
  return ESP_OK;
}
