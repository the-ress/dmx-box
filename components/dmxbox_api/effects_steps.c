#include "api_strings.h"
#include "cJSON.h"
#include "dmxbox_httpd.h"
#include "dmxbox_rest.h"
#include "dmxbox_storage.h"
#include "effect_step_storage.h"
#include "esp_err.h"
#include "serializer.h"
#include <esp_check.h>
#include <esp_http_server.h>
#include <stdint.h>

static const char TAG[] = "dmxbox_api_effect_step";

BEGIN_DMXBOX_API_SERIALIZER(dmxbox_storage_channel_level_t, channel_level)
DMXBOX_API_SERIALIZE_ITEM(
    dmxbox_channel_level_t,
    channel,
    dmxbox_api_channel_to_json,
    dmxbox_api_channel_from_json
)
DMXBOX_API_SERIALIZE_U8(dmxbox_channel_level_t, level)
END_DMXBOX_API_SERIALIZER(dmxbox_channel_level_t, channel_level)

BEGIN_DMXBOX_API_SERIALIZER(dmxbox_storage_effect_step_t, effect_step)
DMXBOX_API_SERIALIZE_U32(dmxbox_effect_step_t, time)
DMXBOX_API_SERIALIZE_U32(dmxbox_effect_step_t, in)
DMXBOX_API_SERIALIZE_U32(dmxbox_effect_step_t, dwell)
DMXBOX_API_SERIALIZE_U32(dmxbox_effect_step_t, out)
DMXBOX_API_SERIALIZE_TRAILING_ARRAY(
    dmxbox_effect_step_t,
    channels,
    channel_count,
    dmxbox_channel_level_to_json,
    dmxbox_channel_level_from_json
)
END_DMXBOX_API_SERIALIZER(dmxbox_effect_step_t, effect_step)

dmxbox_rest_result_t dmxbox_api_effect_step_get(
    httpd_req_t *req,
    uint16_t effect_id,
    uint16_t step_id
) {
  ESP_LOGI(TAG, "GET effect=%u step=%u", effect_id, step_id);

  dmxbox_effect_step_t *effect_step = NULL;
  esp_err_t ret = dmxbox_effect_step_get(effect_id, step_id, &effect_step);
  switch (ret) {
  case ESP_OK:
    break;
  case ESP_ERR_NOT_FOUND:
    return dmxbox_rest_404_not_found;
  default:
    return dmxbox_rest_500_internal_server_error;
  }

  cJSON *json = dmxbox_effect_step_to_json(effect_step);
  free(effect_step);

  if (!json) {
    ESP_LOGE(TAG, "failed to serialize json");
    return dmxbox_rest_500_internal_server_error;
  }

  return dmxbox_rest_result_json(json);
}

dmxbox_rest_result_t dmxbox_api_effect_step_put(
    httpd_req_t *req,
    uint16_t effect_id,
    uint16_t step_id,
    cJSON *json
) {
  ESP_LOGI(TAG, "PUT effect=%u step=%u", effect_id, step_id);
  esp_err_t ret = dmxbox_effect_step_get(effect_id, step_id, NULL);
  switch (ret) {
  case ESP_OK:
    break;
  case ESP_ERR_NOT_FOUND:
    return dmxbox_rest_404_not_found;
  default:
    return dmxbox_rest_500_internal_server_error;
  }

  dmxbox_effect_step_t *parsed = dmxbox_effect_step_from_json_alloc(json);
  if (!parsed) {
    ESP_LOGE(TAG, "failed to parse json");
    return dmxbox_rest_400_bad_request;
  }

  ret = dmxbox_effect_step_set(effect_id, step_id, parsed);
  free(parsed);

  if (ret == ESP_OK) {
    return dmxbox_rest_204_no_content;
  }

  ESP_LOGE(TAG, "failed to save effect step");
  return dmxbox_rest_500_internal_server_error;
}

const dmxbox_rest_container_t effects_steps_router = {
    .slug = "steps",
    .get = dmxbox_api_effect_step_get,
    .post = NULL,
    .put = dmxbox_api_effect_step_put,
};
