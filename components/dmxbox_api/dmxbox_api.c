#include "dmxbox_api.h"
#include "api_config.h"
#include "settings_sta.h"
#include "ws.h"
#include <esp_check.h>
#include <esp_http_server.h>
static const char TAG[] = "dmxbox_api";

esp_err_t dmxbox_api_register(httpd_handle_t server) {
  ESP_RETURN_ON_ERROR(dmxbox_ws_register(server), TAG, "ws register failed");

  ESP_RETURN_ON_ERROR(
      dmxbox_api_settings_sta_register(server),
      TAG,
      "settings_sta register failed"
  );
  ESP_RETURN_ON_ERROR(
      dmxbox_api_config_register(server),
      TAG,
      "api_config register failed"
  );
  return ESP_OK;
}
