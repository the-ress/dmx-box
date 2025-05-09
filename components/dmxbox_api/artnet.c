#include <cJSON.h>
#include <esp_check.h>
#include <esp_err.h>
#include <esp_log.h>
#include <lwip/err.h>
#include <stdio.h>

#include "artnet.h"
#include "dmxbox_artnet.h"
#include "dmxbox_httpd.h"
#include "dmxbox_rest.h"
#include "effect_step_storage.h"
#include "effects_steps.h"

static const char TAG[] = "dmxbox_api_artnet";

static dmxbox_rest_result_t
dmxbox_api_artnet_get(httpd_req_t *req, uint16_t unused, uint16_t universe_id) {
  ESP_LOGI(TAG, "GET artnet=%u", universe_id);

  uint8_t data[DMX_CHANNEL_COUNT];
  if (!dmxbox_artnet_get_universe_data(universe_id, data)) {
    return dmxbox_rest_404_not_found("artnet universe not found");
  }

  uint16_t active_channel_count = 0;
  dmxbox_channel_level_t active_channels[DMX_CHANNEL_COUNT];
  for (uint16_t i = 0; i < DMX_CHANNEL_COUNT; i++) {
    uint8_t level = data[i];
    if (!level) {
      continue;
    }

    active_channels[active_channel_count].channel.index = i + 1;
    active_channels[active_channel_count].channel.universe.address =
        universe_id;
    active_channels[active_channel_count].level = level;
    active_channel_count++;
  }

  cJSON *array = cJSON_CreateArray();
  if (!array) {
    ESP_LOGE(TAG, "failed to allocate array");
    return dmxbox_rest_500_internal_server_error("failed to allocate array");
  }

  for (size_t i = 0; i < active_channel_count; i++) {
    cJSON *json = dmxbox_channel_level_to_json(&active_channels[i]);
    if (!json) {
      ESP_LOGE(
          TAG,
          "failed to serialize channel %u",
          active_channels[i].channel.index
      );
      cJSON_free(array);
      return dmxbox_rest_500_internal_server_error("failed to serialize channel"
      );
    }
    if (!cJSON_AddItemToArray(array, json)) {
      ESP_LOGE(
          TAG,
          "failed to add channel %u to array",
          active_channels[i].channel.index
      );
      cJSON_free(json);
      cJSON_free(array);
      return dmxbox_rest_500_internal_server_error(
          "failed to add channel to array"
      );
    }
  }

  return dmxbox_rest_result_json(array);
}

static esp_err_t dmxbox_api_artnet_clear(httpd_req_t *req) {
  ESP_LOGI(TAG, "POST request for %s", req->uri);
  dmxbox_httpd_cors_allow_origin(req);

  dmxbox_artnet_reset_state();

  ESP_RETURN_ON_ERROR(
      httpd_resp_set_status(req, HTTPD_204),
      TAG,
      "failed to set status"
  );

  ESP_RETURN_ON_ERROR(
      httpd_resp_send_chunk(req, NULL, 0),
      TAG,
      "failed to send empty chunk"
  );

  return ERR_OK;
}

static const dmxbox_rest_container_t artnet_router = {
    .slug = "artnet",
    .allow_zero = true,
    .get = dmxbox_api_artnet_get,
};

esp_err_t dmxbox_api_artnet_register(httpd_handle_t server) {
  static const httpd_uri_t clear = {
      .uri = "/api/artnet/clear",
      .method = HTTP_POST,
      .handler = dmxbox_api_artnet_clear,
  };
  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &clear),
      TAG,
      "artnet/clear register failed"
  );

  ESP_RETURN_ON_ERROR(
      dmxbox_rest_register(server, &artnet_router),
      TAG,
      "artnet register failed"
  );

  return ESP_OK;
}
