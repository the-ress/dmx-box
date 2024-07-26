#include <cJSON.h>
#include <esp_check.h>
#include <esp_err.h>
#include <esp_log.h>
#include <stdio.h>

#include "artnet.h"
#include "dmxbox_artnet.h"
#include "dmxbox_dmx_receive.h"
#include "dmxbox_dmx_send.h"
#include "dmxbox_httpd.h"
#include "dmxbox_rest.h"
#include "dmxbox_storage.h"
#include "effect_step_storage.h"
#include "effects_steps.h"

static const char TAG[] = "dmxbox_api_dmx";

static dmxbox_rest_result_t serialize_active_channels(
    uint8_t data[DMX_CHANNEL_COUNT],
    uint16_t universe_address
) {
  uint16_t active_channel_count = 0;
  dmxbox_channel_level_t active_channels[DMX_CHANNEL_COUNT];
  for (uint16_t i = 0; i < DMX_CHANNEL_COUNT; i++) {
    uint8_t level = data[i];
    if (!level) {
      continue;
    }

    active_channels[active_channel_count].channel.index = i + 1;
    active_channels[active_channel_count].channel.universe.address =
        universe_address;
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

static esp_err_t dmxbox_api_dmx_output_get(httpd_req_t *req) {
  ESP_LOGI(TAG, "GET request for %s", req->uri);
  dmxbox_httpd_cors_allow_origin(req);

  uint8_t data[DMX_CHANNEL_COUNT];
  dmxbox_dmx_send_get_data(data);

  dmxbox_rest_result_t result =
      serialize_active_channels(data, dmxbox_get_native_universe());
  return dmxbox_rest_send(req, result);
}

static esp_err_t dmxbox_api_dmx_input_get(httpd_req_t *req) {
  ESP_LOGI(TAG, "GET request for %s", req->uri);
  dmxbox_httpd_cors_allow_origin(req);

  uint8_t data[DMX_CHANNEL_COUNT];
  dmxbox_dmx_receive_get_data(data);

  dmxbox_rest_result_t result =
      serialize_active_channels(data, dmxbox_get_native_universe());
  return dmxbox_rest_send(req, result);
}

esp_err_t dmxbox_api_dmx_register(httpd_handle_t server) {
  static const httpd_uri_t output = {
      .uri = "/api/dmx/output",
      .method = HTTP_GET,
      .handler = dmxbox_api_dmx_output_get,
  };
  static const httpd_uri_t input = {
      .uri = "/api/dmx/input",
      .method = HTTP_GET,
      .handler = dmxbox_api_dmx_input_get,
  };
  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &output),
      TAG,
      "dmx/output register failed"
  );
  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &input),
      TAG,
      "dmx/input register failed"
  );
  return ESP_OK;
}
