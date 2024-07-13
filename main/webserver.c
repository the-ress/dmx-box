#include "dmxbox_api.h"
#include "dmxbox_httpd.h"
#include <esp_check.h>
#include <esp_http_server.h>
#include <esp_log.h>

static const char *TAG = "dmxbox_webserver";

esp_err_t dmxbox_webserver_start() {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.task_priority = 1;
  config.max_open_sockets = CONFIG_DMXBOX_WEBSEVER_MAX_SOCKETS;
  config.max_uri_handlers = 32;
  config.lru_purge_enable = true;
  config.uri_match_fn = httpd_uri_match_wildcard;

  // Start the httpd server
  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  ESP_RETURN_ON_ERROR(
      httpd_start(&server, &config),
      __func__,
      "httpd_start failed"
  );

  ESP_LOGI(TAG, "Registering URI handlers");

  ESP_RETURN_ON_ERROR(
      dmxbox_api_register(server),
      TAG,
      "api_config_register failed"
  );
  ESP_RETURN_ON_ERROR(
      dmxbox_httpd_statics_register(server),
      TAG,
      "static register failed"
  );

  return ESP_OK;
}
