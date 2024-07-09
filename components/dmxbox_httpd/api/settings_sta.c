#include "settings_sta.h"
#include <esp_check.h>
#include <esp_http_server.h>

static const char TAG[] = "dmxbox_api_settings_sta";

static esp_err_t dmxbox_api_settings_sta_get(httpd_req_t *req) {
  return ESP_OK;
}

static esp_err_t dmxbox_api_settings_sta_put(httpd_req_t *req) {
  return ESP_OK;
}

esp_err_t dmxbox_api_settings_sta_register(httpd_handle_t server) {
  static const httpd_uri_t get = {
      .uri = "/api/settings/sta",
      .method = HTTP_GET,
      .handler = dmxbox_api_settings_sta_get,
  };
  static const httpd_uri_t put = {
      .uri = "/api/settings/sta",
      .method = HTTP_PUT,
      .handler = dmxbox_api_settings_sta_put,
  };
  ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &get), TAG,
                      "handler_config_get failed");
  ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &put), TAG,
                      "handler_config_put failed");
  return ESP_OK;
}
