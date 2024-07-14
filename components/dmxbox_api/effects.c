#include "effects.h"
#include "api_strings.h"
#include "cJSON.h"
#include "dmxbox_httpd.h"
#include "dmxbox_storage.h"
#include "effect_storage.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_http_server.h"
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
DMXBOX_API_SERIALIZE_TRAILING_ARRAY(
    dmxbox_effect_t,
    steps,
    step_count,
    dmxbox_u16_to_json,
    dmxbox_u16_from_json
)
END_DMXBOX_API_SERIALIZER(dmxbox_effect_t, effect)

static esp_err_t dmxbox_api_effect_post(httpd_req_t *req) {
  ESP_LOGI(TAG, "POST effect");

  cJSON *json = NULL;
  ESP_RETURN_ON_ERROR(
      dmxbox_httpd_receive_json(req, &json),
      TAG,
      "failed to receive json"
  );

  dmxbox_effect_t *parsed = dmxbox_effect_from_json_alloc(json);
  cJSON_free(json);

  if (!parsed) {
    ESP_LOGE(TAG, "failed to parse json");
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
  }

  uint16_t id;
  esp_err_t err = dmxbox_effect_create(parsed, &id);
  free(parsed);

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "failed to create effect");
    return httpd_resp_send_500(req);
  }

  char location[sizeof("/api/effects/65536")];
  if (snprintf(location, sizeof(location), "/api/effects/%u", id) >=
      sizeof(location)) {
    ESP_LOGE(TAG, "failed to create Location header");
    return httpd_resp_send_500(req);
  }

  ESP_RETURN_ON_ERROR(
      httpd_resp_set_hdr(req, "Location", location),
      TAG,
      "failed to set Location"
  );

  return dmxbox_httpd_send_201_created(req);
}

static esp_err_t dmxbox_api_effect_get(httpd_req_t *req, uint16_t effect_id) {
  ESP_LOGI(TAG, "GET effect=%u", effect_id);

  dmxbox_effect_t *effect = NULL;
  esp_err_t ret = dmxbox_effect_get(effect_id, &effect);
  switch (ret) {
  case ESP_OK:
    break;
  case ESP_ERR_NOT_FOUND:
    return httpd_resp_send_404(req);
  default:
    return ret;
  }

  cJSON *json = dmxbox_effect_to_json(effect);
  if (!json) {
    ESP_LOGE(TAG, "failed to serialize json");
    ret = httpd_resp_send_500(req);
    goto exit;
  }

  ESP_GOTO_ON_ERROR(
      dmxbox_httpd_send_json(req, json),
      exit,
      TAG,
      "failed to send json"
  );

exit:
  if (effect) {
    free(effect);
  }
  if (json) {
    cJSON_free(json);
  }
  return ret;
}

static esp_err_t dmxbox_api_effect_list(httpd_req_t *req) {
  ESP_LOGI(TAG, "GET effects");

  dmxbox_effect_entry_t effects[30];
  uint16_t count = sizeof(effects) / sizeof(effects[0]);
  esp_err_t ret = ESP_OK;
  cJSON *array = NULL;

  ESP_GOTO_ON_ERROR(
      dmxbox_effect_list(0, &count, effects),
      exit,
      TAG,
      "failed to list effects"
  );

  array = cJSON_CreateArray();
  if (!array) {
    ESP_LOGE(TAG, "failed to allocate array");
    ret = ESP_ERR_NO_MEM;
    goto exit;
  }

  for (size_t i = 0; i < count; i++) {
    cJSON *json = dmxbox_effect_to_json(effects[i].effect);
    if (!json) {
      ESP_LOGE(TAG, "failed to serialize effect %u", effects[i].id);
      ret = httpd_resp_send_500(req);
      goto exit;
    }
    if (!cJSON_AddNumberToObject(json, "id", effects[i].id)) {
      ESP_LOGE(TAG, "failed to add id for %u", effects[i].id);
      ret = httpd_resp_send_500(req);
      cJSON_free(json);
      goto exit;
    }
    if (!cJSON_AddItemToArray(array, json)) {
      ESP_LOGE(TAG, "failed to add effect %u to array", effects[i].id);
      ret = httpd_resp_send_500(req);
      cJSON_free(json);
      goto exit;
    }
  }

  ESP_GOTO_ON_ERROR(
      dmxbox_httpd_send_json(req, array),
      exit,
      TAG,
      "failed to send json"
  );

exit:
  cJSON_free(array);
  while (count--) {
    free(effects[count].effect);
  }
  return ret;
}

esp_err_t dmxbox_api_effect_container_endpoint(httpd_req_t *req) {
  if (req->method == HTTP_GET) {
    return dmxbox_api_effect_list(req);
  } else if (req->method == HTTP_POST) {
    return dmxbox_api_effect_post(req);
  }
  return httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, NULL);
}

esp_err_t dmxbox_api_effect_endpoint(httpd_req_t *req, uint16_t effect_id) {
  if (req->method == HTTP_GET) {
    return dmxbox_api_effect_get(req, effect_id);
  } else if (req->method == HTTP_PUT) {
    // return dmxbox_api_effect_put(req, effect_id);
  }
  return httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, NULL);
}
