#include "dmxbox_api.h"
#include "api_config.h"
#include "dmxbox_httpd.h"
#include "effects_steps.h"
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
  ESP_RETURN_ON_ERROR(
      dmxbox_api_effect_steps_register(server),
      TAG,
      "effects steps register failed"
  );
  ESP_RETURN_ON_ERROR(
      dmxbox_httpd_cors_register_options(server, "/api/*"),
      TAG,
      "failed to register /api OPTIONS handler"
  );

  return ESP_OK;
}
