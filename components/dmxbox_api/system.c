#include <cJSON.h>
#include <esp_check.h>
#include <esp_err.h>
#include <esp_http_server.h>

#include "api_strings.h"
#include "dmxbox_artnet.h"
#include "dmxbox_httpd.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "settings_artnet.h"

static const char TAG[] = "dmxbox_api_system";

static esp_err_t dmxbox_api_system_uptime(httpd_req_t *req) {
  ESP_LOGI(TAG, "GET request for %s", req->uri);

  dmxbox_httpd_cors_allow_origin(req);
  int64_t uptime = esp_timer_get_time() / 1000000;
  ESP_LOGI(TAG, "uptime is %lld seconds", uptime);

  esp_err_t ret = ESP_ERR_NO_MEM;
  cJSON *json = cJSON_CreateObject();
  if (!json) {
    goto exit;
  }

  if (!cJSON_AddNumberToObject(json, "uptime", uptime)) {
    goto exit;
  }
  ret = dmxbox_httpd_send_json(req, json);
exit:
  if (json) {
    cJSON_free(json);
  }
  return ret;
}

static esp_err_t dmxbox_api_system_reboot(httpd_req_t *req) {
  ESP_LOGI(TAG, "POST request for %s", req->uri);

  dmxbox_httpd_cors_allow_origin(req);

  ESP_LOGI(TAG, "saving artnet state");
  dmxbox_artnet_save_universe_snapshots();

  ESP_LOGI(TAG, "rebooting");
  esp_restart();
  // TODO reboot more gracefully - artnet is logging some errors
}

esp_err_t dmxbox_api_system_register(httpd_handle_t server) {
  static const httpd_uri_t uptime = {
      .uri = "/api/system/uptime",
      .method = HTTP_GET,
      .handler = dmxbox_api_system_uptime,
  };
  static const httpd_uri_t reboot = {
      .uri = "/api/system/reboot",
      .method = HTTP_POST,
      .handler = dmxbox_api_system_reboot,
  };
  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &uptime),
      TAG,
      "system/uptime register failed"
  );
  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &reboot),
      TAG,
      "system/reboot register failed"
  );
  return ESP_OK;
}
