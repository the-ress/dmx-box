#include <esp_check.h>
#include <esp_http_server.h>

#include "api_config.h"
#include "artnet.h"
#include "dmx.h"
#include "dmxbox_api.h"
#include "dmxbox_httpd.h"
#include "dmxbox_rest.h"
#include "effects.h"
#include "settings_artnet.h"
#include "settings_sta.h"
#include "system.h"
#include "ws.h"

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
      dmxbox_api_settings_artnet_register(server),
      TAG,
      "settings_artnet register failed"
  );
  ESP_RETURN_ON_ERROR(
      dmxbox_api_system_register(server),
      TAG,
      "system register failed"
  );
  ESP_RETURN_ON_ERROR(
      dmxbox_api_artnet_register(server),
      TAG,
      "artnet register failed"
  );
  ESP_RETURN_ON_ERROR(
      dmxbox_api_dmx_register(server),
      TAG,
      "dmx register failed"
  );
  ESP_RETURN_ON_ERROR(
      dmxbox_rest_register(server, &effects_router),
      TAG,
      "effects register failed"
  );
  ESP_RETURN_ON_ERROR(
      dmxbox_httpd_cors_register_options(server, "/api/*"),
      TAG,
      "failed to register /api OPTIONS handler"
  );

  return ESP_OK;
}
