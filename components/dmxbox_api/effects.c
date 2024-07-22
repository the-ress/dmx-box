#include "effects.h"
#include "api_strings.h"
#include "cJSON.h"
#include "dmxbox_httpd.h"
#include "dmxbox_rest.h"
#include "dmxbox_storage.h"
#include "effect_storage.h"
#include "effects_steps.h"
#include "esp_check.h"
#include "esp_err.h"
#include "serializer.h"
#include <esp_log.h>
#include <stdio.h>

static const char TAG[] = "dmxbox_api_effect";

BEGIN_DMXBOX_API_SERIALIZER(dmxbox_effect_t, effect)
DMXBOX_API_SERIALIZE_ITEM(
    dmxbox_effect_t,
    level_channel,
    dmxbox_api_channel_to_json,
    dmxbox_api_channel_from_json
)
DMXBOX_API_SERIALIZE_ITEM(
    dmxbox_effect_t,
    rate_channel,
    dmxbox_api_channel_to_json,
    dmxbox_api_channel_from_json
)
DMXBOX_API_SERIALIZE_U16(dmxbox_effect_t, distributed_id)
DMXBOX_API_SERIALIZE_TRAILING_ARRAY(
    dmxbox_effect_t,
    steps,
    step_count,
    dmxbox_u16_ptr_to_json,
    dmxbox_u16_from_json
)
END_DMXBOX_API_SERIALIZER(dmxbox_effect_t, effect)

static dmxbox_rest_result_t dmxbox_api_effect_post(
    httpd_req_t *req,
    uint16_t unused_parent_id,
    cJSON *json
) {
  ESP_LOGI(TAG, "POST effect");

  dmxbox_effect_t *parsed = dmxbox_effect_from_json_alloc(json);
  if (!parsed) {
    ESP_LOGE(TAG, "failed to parse effect");
    return dmxbox_rest_400_bad_request;
  }

  uint16_t id;
  esp_err_t err = dmxbox_effect_create(parsed, &id);
  free(parsed);

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "failed to create effect");
    return dmxbox_rest_500_internal_server_error;
  }

  return dmxbox_rest_201_created("/api/effects/%u", id);
}

dmxbox_rest_result_t dmxbox_api_effect_put(
    httpd_req_t *req,
    uint16_t unused_parent_id,
    uint16_t effect_id,
    cJSON *json
) {
  ESP_LOGI(TAG, "PUT effect=%u", effect_id);
  esp_err_t ret = dmxbox_effect_get(effect_id, NULL);
  switch (ret) {
  case ESP_OK:
    break;
  case ESP_ERR_NOT_FOUND:
    return dmxbox_rest_404_not_found;
  default:
    return dmxbox_rest_500_internal_server_error;
  }

  dmxbox_effect_t *parsed = dmxbox_effect_from_json_alloc(json);
  if (!parsed) {
    ESP_LOGE(TAG, "failed to parse effect");
    return dmxbox_rest_400_bad_request;
  }

  ret = dmxbox_effect_set(effect_id, parsed);
  free(parsed);

  if (ret == ESP_OK) {
    return dmxbox_rest_204_no_content;
  }

  ESP_LOGE(TAG, "failed to save effect");
  return dmxbox_rest_500_internal_server_error;
}

static dmxbox_rest_result_t
dmxbox_api_effect_get(httpd_req_t *req, uint16_t unused, uint16_t effect_id) {
  ESP_LOGI(TAG, "GET effect=%u", effect_id);

  dmxbox_effect_t *effect = NULL;
  esp_err_t ret = dmxbox_effect_get(effect_id, &effect);
  switch (ret) {
  case ESP_OK:
    break;
  case ESP_ERR_NOT_FOUND:
    return dmxbox_rest_404_not_found;
  default:
    return dmxbox_rest_500_internal_server_error;
  }

  cJSON *json = dmxbox_effect_to_json(effect);
  free(effect);

  if (!json) {
    ESP_LOGE(TAG, "failed to serialize json");
    return dmxbox_rest_500_internal_server_error;
  }

  return dmxbox_rest_result_json(json);
}

static dmxbox_rest_result_t dmxbox_api_effect_delete(
    httpd_req_t *req,
    uint16_t unused,
    uint16_t effect_id
) {
  ESP_LOGI(TAG, "DELETE effect=%u", effect_id);

  esp_err_t ret = dmxbox_effect_delete(effect_id);
  switch (ret) {
  case ESP_OK:
    return dmxbox_rest_200_ok;
  case ESP_ERR_NOT_FOUND:
    return dmxbox_rest_404_not_found;
  default:
    return dmxbox_rest_500_internal_server_error;
  }
}

static dmxbox_rest_result_t
dmxbox_api_effect_list(httpd_req_t *req, uint16_t unused_parent_id) {
  ESP_LOGI(TAG, "GET effects");

  dmxbox_rest_result_t result = dmxbox_rest_500_internal_server_error;

  cJSON *array = cJSON_CreateArray();
  if (!array) {
    ESP_LOGE(TAG, "failed to allocate array");
    return result;
  }

  dmxbox_storage_entry_t effects[30];
  uint16_t count = sizeof(effects) / sizeof(effects[0]);
  if (dmxbox_effect_list(0, &count, effects) != ESP_OK) {
    ESP_LOGE(TAG, "failed to list effects");
    cJSON_free(array);
    return result;
  }

  for (size_t i = 0; i < count; i++) {
    cJSON *json = dmxbox_effect_to_json(effects[i].data);
    if (!json) {
      ESP_LOGE(TAG, "failed to serialize effect %u", effects[i].id);
      cJSON_free(array);
      goto exit;
    }
    if (!cJSON_AddNumberToObject(json, "id", effects[i].id)) {
      ESP_LOGE(TAG, "failed to add id for %u", effects[i].id);
      cJSON_free(json);
      cJSON_free(array);
      goto exit;
    }
    if (!cJSON_AddItemToArray(array, json)) {
      ESP_LOGE(TAG, "failed to add effect %u to array", effects[i].id);
      cJSON_free(json);
      cJSON_free(array);
      goto exit;
    }
  }

  result = dmxbox_rest_result_json(array);

exit:
  while (count--) {
    free(effects[count].data);
  }
  return result;
}

static const dmxbox_rest_child_container_t effect_step_child_router = {
    .container = &effects_steps_router};

const dmxbox_rest_container_t effects_router = {
    .slug = "effects",
    .get = dmxbox_api_effect_get,
    .post = dmxbox_api_effect_post,
    .put = dmxbox_api_effect_put,
    .delete = dmxbox_api_effect_delete,
    .list = dmxbox_api_effect_list,
    .first_child = &effect_step_child_router,
};
