#include "cJSON.h"
#include "dmxbox_httpd.h"
#include "dmxbox_storage.h"
#include "esp_err.h"
#include <esp_check.h>
#include <esp_http_server.h>
#include <stdint.h>

static const char TAG[] = "dmxbox_api_effect_step";

static cJSON *dmxbox_api_channel_to_json(dmxbox_storage_channel_t c) {
  char buffer[sizeof("127-16-16/512")];
  size_t size = c.universe.address
                    ? snprintf(
                          buffer,
                          sizeof(buffer),
                          "%u-%u-%u/%u",
                          c.universe.net,
                          c.universe.subnet,
                          c.universe.universe,
                          c.index
                      )
                    : snprintf(buffer, sizeof(buffer), "%u", c.index);
  return size < sizeof(buffer) ? cJSON_CreateString(buffer) : NULL;
}

typedef struct dmxbox_serializer_entry {
  int cjson_type;
  const char *name;
  size_t offset;
} dmxbox_serializer_entry_t;

#define BEGIN_DMXBOX_API_SERIALIZER(name, type)                                \
  static cJSON *dmxbox_api_##name##_to_json(const type *object) {              \
    cJSON *json = cJSON_CreateObject();                                        \
    if (!json) {                                                               \
      return NULL;                                                             \
    }

#define DMXBOX_API_SERIALIZE_NUMBER(name)                                      \
  if (!cJSON_AddNumberToObject(json, #name, object->name)) {                   \
    goto error;                                                                \
  }

#define DMXBOX_API_SERIALIZE_ITEM(name, serializer)                            \
  do {                                                                         \
    cJSON *item = serializer(object->name);                                    \
    if (!item) {                                                               \
      goto error;                                                              \
    }                                                                          \
    if (!cJSON_AddItemToObjectCS(json, #name, item)) {                         \
      cJSON_free(item);                                                        \
      goto error;                                                              \
    }                                                                          \
  } while (false)

#define END_DMXBOX_API_SERIALIZER()                                            \
  return json;                                                                 \
  error:                                                                       \
  cJSON_free(json);                                                            \
  return NULL;                                                                 \
  }

BEGIN_DMXBOX_API_SERIALIZER(channel_level, dmxbox_storage_channel_level_t)
DMXBOX_API_SERIALIZE_ITEM(channel, dmxbox_api_channel_to_json);
DMXBOX_API_SERIALIZE_NUMBER(level);
END_DMXBOX_API_SERIALIZER()

BEGIN_DMXBOX_API_SERIALIZER(effect_step_header, dmxbox_storage_effect_step_t)
DMXBOX_API_SERIALIZE_NUMBER(time);
DMXBOX_API_SERIALIZE_NUMBER(in);
DMXBOX_API_SERIALIZE_NUMBER(dwell);
DMXBOX_API_SERIALIZE_NUMBER(out);
END_DMXBOX_API_SERIALIZER()

static cJSON *dmxbox_api_effect_step_to_json(
    const dmxbox_storage_effect_step_t *object,
    size_t channel_count
) {
  cJSON *json = dmxbox_api_effect_step_header_to_json(object);

  cJSON *channels = cJSON_AddArrayToObject(json, "channels");
  if (!channels) {
    goto exit;
  }

  for (size_t i = 0; i < channel_count; i++) {
    cJSON *channel_level =
        dmxbox_api_channel_level_to_json(&object->channels[i]);
    if (!cJSON_AddItemToArray(channels, channel_level)) {
      cJSON_free(channel_level);
      goto exit;
    }
  }
  return json;

exit:
  cJSON_free(json);
  return NULL;
}

esp_err_t dmxbox_api_effect_step_get(
    httpd_req_t *req,
    uint16_t effect_id,
    uint16_t step_id
) {
  esp_err_t ret = ESP_OK;
  size_t channel_count = 0;
  dmxbox_storage_effect_step_t *effect_step = NULL;
  cJSON *json = NULL;

  ret = dmxbox_storage_effect_step_get(
      effect_id,
      step_id,
      &channel_count,
      &effect_step
  );

  if (ret == ESP_ERR_NOT_FOUND) {
    ESP_GOTO_ON_ERROR(
        httpd_resp_send_404(req),
        exit,
        TAG,
        "failed to send 404"
    );
    return ESP_OK;
  }
  ESP_GOTO_ON_ERROR(ret, exit, TAG, "failed to read effect step from storage");

  json = dmxbox_api_effect_step_to_json(effect_step, channel_count);
  if (!json) {
    ESP_LOGE(TAG, "failed to serialize json");
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

esp_err_t dmxbox_api_effect_step_endpoint(
    httpd_req_t *req,
    uint16_t effect_id,
    uint16_t step_id
) {
  if (req->method == HTTP_GET) {
    return dmxbox_api_effect_step_get(req, effect_id, step_id);
  } else if (req->method == HTTP_PUT) {
    ESP_RETURN_ON_ERROR(
        httpd_resp_set_status(req, "405 Method Not Allowed"),
        TAG,
        "failed to send 405"
    );
    return httpd_resp_send(req, NULL, 0);
  }
  return ESP_OK;
}
