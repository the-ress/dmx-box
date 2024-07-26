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
    return dmxbox_rest_404_not_found("effect step not found");
  default:
    return dmxbox_rest_500_internal_server_error(
        "failed to get the effect step from storage"
    );
  }

  cJSON *json = dmxbox_effect_step_to_json(effect_step);
  free(effect_step);

  if (!json) {
    ESP_LOGE(TAG, "failed to serialize json");
    return dmxbox_rest_500_internal_server_error("failed to serialize json");
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
  // esp_err_t ret = dmxbox_effect_step_get(effect_id, step_id, NULL);
  // switch (ret) {
  // case ESP_OK:
  //   break;
  // case ESP_ERR_NOT_FOUND:
  //   return dmxbox_rest_404_not_found("effect step not found");
  // default:
  //   return dmxbox_rest_500_internal_server_error(
  //       "failed to get the effect step from storage"
  //   );
  // }

  dmxbox_effect_step_t *parsed = dmxbox_effect_step_from_json_alloc(json);
  if (!parsed) {
    ESP_LOGE(TAG, "failed to parse json");
    return dmxbox_rest_400_bad_request("failed to parse effect step");
  }

  esp_err_t ret = dmxbox_effect_step_set(effect_id, step_id, parsed);
  free(parsed);

  if (ret == ESP_OK) {
    return dmxbox_rest_204_no_content;
  }

  ESP_LOGE(TAG, "failed to save effect step");
  return dmxbox_rest_500_internal_server_error(
      "failed to save effect step to storage"
  );
}

dmxbox_rest_result_t dmxbox_api_effect_step_batch_put(
    httpd_req_t *req,
    uint16_t effect_id,
    cJSON *json
) {
  ESP_LOGI(TAG, "PUT effect=%u steps", effect_id);
  // TODO delete steps not present in the request

  if (!cJSON_IsArray(json)) {
    ESP_LOGE(TAG, "failed to parse input: not an array");
    return dmxbox_rest_400_bad_request("failed to parse input: not an array");
  }

  ESP_LOGI(TAG, "got %u steps", cJSON_GetArraySize(json));

  cJSON *step_json;
  int i = 0;
  cJSON_ArrayForEach(step_json, json) {
    cJSON *step_id_json = cJSON_GetObjectItem(step_json, "id");
    if (!step_id_json) {
      ESP_LOGE(TAG, "step id is missing at position %d", i);
      return dmxbox_rest_400_bad_request("step id is missing");
    }

    uint16_t step_id;
    if (!dmxbox_u16_from_json(step_id_json, &step_id)) {
      ESP_LOGE(TAG, "failed parse step id at position %d", i);
      return dmxbox_rest_400_bad_request("failed to parse step id");
    }

    dmxbox_effect_step_t *parsed =
        dmxbox_effect_step_from_json_alloc(step_json);
    if (!parsed) {
      ESP_LOGE(
          TAG,
          "failed to parse effect step %u at position %d",
          step_id,
          i
      );
      return dmxbox_rest_400_bad_request("failed to parse effect step");
    }

    esp_err_t ret = dmxbox_effect_step_set(effect_id, step_id, parsed);
    free(parsed);

    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "failed to save effect step %u", step_id);
      return dmxbox_rest_500_internal_server_error(
          "failed to save effect step to storage"
      );
    }

    i++;
  }

  return dmxbox_rest_204_no_content;
}

static dmxbox_rest_result_t dmxbox_api_effect_step_delete(
    httpd_req_t *req,
    uint16_t effect_id,
    uint16_t step_id
) {
  ESP_LOGI(TAG, "DELETE effect=%u step=%u", effect_id, step_id);

  esp_err_t ret = dmxbox_effect_step_delete(effect_id, step_id);
  switch (ret) {
  case ESP_OK:
    return dmxbox_rest_200_ok;
  case ESP_ERR_NOT_FOUND:
    return dmxbox_rest_404_not_found("effect step not found");
  default:
    return dmxbox_rest_500_internal_server_error(
        "failed to delete the effect step to storage"
    );
  }
}

static dmxbox_rest_result_t
dmxbox_api_effect_step_list(httpd_req_t *req, uint16_t effect_id) {
  ESP_LOGI(TAG, "GET effect=%u steps", effect_id);

  cJSON *array = cJSON_CreateArray();
  if (!array) {
    ESP_LOGE(TAG, "failed to allocate array");
    return dmxbox_rest_500_internal_server_error("failed to allocate array");
  }

  dmxbox_storage_entry_t steps[30];
  uint16_t count = sizeof(steps) / sizeof(steps[0]);
  if (dmxbox_effect_step_list(effect_id, 0, &count, steps) != ESP_OK) {
    ESP_LOGE(TAG, "failed to list steps");
    cJSON_free(array);
    return dmxbox_rest_500_internal_server_error("failed to list steps");
  }

  dmxbox_rest_result_t result;
  for (size_t i = 0; i < count; i++) {
    cJSON *json = dmxbox_effect_step_to_json(steps[i].data);
    if (!json) {
      ESP_LOGE(
          TAG,
          "failed to serialize effect %u step %u",
          effect_id,
          steps[i].id
      );
      cJSON_free(array);
      result = dmxbox_rest_500_internal_server_error(
          "failed to serialize effect step"
      );
      goto exit;
    }
    if (!cJSON_AddNumberToObject(json, "id", steps[i].id)) {
      ESP_LOGE(TAG, "failed to add id for %u", steps[i].id);
      cJSON_free(json);
      cJSON_free(array);
      result = dmxbox_rest_500_internal_server_error(
          "failed to add id to effect step object"
      );
      goto exit;
    }
    if (!cJSON_AddItemToArray(array, json)) {
      ESP_LOGE(
          TAG,
          "failed to add effect %u step %u to array",
          effect_id,
          steps[i].id
      );
      cJSON_free(json);
      cJSON_free(array);
      result = dmxbox_rest_500_internal_server_error(
          "failed to add effect step to array"
      );
      goto exit;
    }
  }

  result = dmxbox_rest_result_json(array);

exit:
  while (count--) {
    free(steps[count].data);
  }
  return result;
}

const dmxbox_rest_container_t effects_steps_router = {
    .slug = "steps",
    .get = dmxbox_api_effect_step_get,
    .post = NULL,
    .put = dmxbox_api_effect_step_put,
    .batch_put = dmxbox_api_effect_step_batch_put,
    .delete = dmxbox_api_effect_step_delete,
    .list = dmxbox_api_effect_step_list,
};
