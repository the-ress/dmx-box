#include "cJSON.h"
#include "dmxbox_httpd.h"
#include "dmxbox_storage.h"
#include "effect_step_storage.h"
#include "esp_err.h"
#include "serializer.h"
#include <esp_check.h>
#include <esp_http_server.h>
#include <stdint.h>

static const char TAG[] = "dmxbox_api_effect_step";

static cJSON *dmxbox_api_channel_to_json(const dmxbox_storage_channel_t *c) {
  char buffer[sizeof("127-16-16/512")];
  size_t size = c->universe.address
                    ? snprintf(
                          buffer,
                          sizeof(buffer),
                          "%u-%u-%u/%u",
                          c->universe.net,
                          c->universe.subnet,
                          c->universe.universe,
                          c->index
                      )
                    : snprintf(buffer, sizeof(buffer), "%u", c->index);
  return size < sizeof(buffer) ? cJSON_CreateString(buffer) : NULL;
}

static bool
dmxbox_api_channel_from_json(const cJSON *json, dmxbox_storage_channel_t *c) {
  if (!cJSON_IsString(json)) {
    ESP_LOGE(TAG, "channel is not a string");
    return false;
  }

  // TODO non-zero universes
  int value = atoi(json->valuestring);
  if (value < 1 || value > 512) {
    ESP_LOGE(TAG, "channel out of range: %d", value);
    return false;
  }

  c->index = value;
  return true;
}

BEGIN_DMXBOX_API_SERIALIZER(dmxbox_storage_channel_level_t, channel_level)
DMXBOX_API_SERIALIZE_ITEM(
    dmxbox_storage_channel_level_t,
    channel,
    dmxbox_api_channel_to_json,
    dmxbox_api_channel_from_json
)
DMXBOX_API_SERIALIZE_U8(dmxbox_storage_channel_level_t, level)
END_DMXBOX_API_SERIALIZER(dmxbox_storage_channel_level_t, channel_level)

BEGIN_DMXBOX_API_SERIALIZER(dmxbox_storage_effect_step_t, effect_step)
DMXBOX_API_SERIALIZE_U32(dmxbox_storage_effect_step_t, time)
DMXBOX_API_SERIALIZE_U32(dmxbox_storage_effect_step_t, in)
DMXBOX_API_SERIALIZE_U32(dmxbox_storage_effect_step_t, dwell)
DMXBOX_API_SERIALIZE_U32(dmxbox_storage_effect_step_t, out)
DMXBOX_API_SERIALIZE_TRAILING_ARRAY(
    dmxbox_storage_effect_step_t,
    channels,
    channel_count,
    dmxbox_channel_level_to_json,
    dmxbox_channel_level_from_json
)
END_DMXBOX_API_SERIALIZER(dmxbox_storage_effect_step_t, effect_step)

esp_err_t dmxbox_api_effect_step_get(
    httpd_req_t *req,
    uint16_t effect_id,
    uint16_t step_id
) {
  ESP_LOGI(TAG, "GET effect=%u step=%u", effect_id, step_id);

  dmxbox_storage_effect_step_t *effect_step = NULL;
  esp_err_t ret =
      dmxbox_storage_effect_step_get(effect_id, step_id, &effect_step);
  switch (ret) {
  case ESP_OK:
    break;
  case ESP_ERR_NOT_FOUND:
    return httpd_resp_send_404(req);
  default:
    return ret;
  }

  cJSON *json = dmxbox_effect_step_to_json(effect_step);
  if (!json) {
    ESP_LOGE(TAG, "failed to serialize json");
    ret = ESP_FAIL;
    goto exit;
  }

  ESP_GOTO_ON_ERROR(
      dmxbox_httpd_send_json(req, json),
      exit,
      TAG,
      "failed to send json"
  );

exit:
  if (effect_step) {
    free(effect_step);
  }
  if (json) {
    cJSON_free(json);
  }
  return ret;
}

esp_err_t dmxbox_api_effect_step_put(
    httpd_req_t *req,
    uint16_t effect_id,
    uint16_t step_id
) {
  ESP_LOGI(TAG, "PUT effect=%u step=%u", effect_id, step_id);
  esp_err_t ret = dmxbox_storage_effect_step_get(effect_id, step_id, NULL);
  switch (ret) {
  case ESP_OK:
    break;
  case ESP_ERR_NOT_FOUND:
    return httpd_resp_send_404(req);
  default:
    return ret;
  }

  cJSON *json = NULL;
  ESP_RETURN_ON_ERROR(
      dmxbox_httpd_receive_json(req, &json),
      TAG,
      "failed to receive json"
  );

  dmxbox_storage_effect_step_t *parsed =
      dmxbox_effect_step_from_json_alloc(json);
  cJSON_free(json);

  if (!parsed) {
    ESP_LOGE(TAG, "failed to parse json");
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
  }

  ret = dmxbox_storage_effect_step_set(effect_id, step_id, parsed);
  free(parsed);

  if (ret == ESP_OK) {
    return dmxbox_httpd_send_204_no_content(req);
  }

  ESP_LOGE(TAG, "failed to save effect step");
  return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, NULL);
}

esp_err_t dmxbox_api_effect_step_endpoint(
    httpd_req_t *req,
    uint16_t effect_id,
    uint16_t step_id
) {
  if (req->method == HTTP_GET) {
    return dmxbox_api_effect_step_get(req, effect_id, step_id);
  } else if (req->method == HTTP_PUT) {
    return dmxbox_api_effect_step_put(req, effect_id, step_id);
  }
  return httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, NULL);
}
