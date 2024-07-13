#include "cJSON.h"
#include "dmxbox_httpd.h"
#include "dmxbox_storage.h"
#include "esp_err.h"
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

struct dmxbox_serializer_entry;

typedef cJSON *(*dmxbox_ptr_to_json_func_t)(const void *object);

typedef cJSON *(*dmxbox_serialize_func_t)(
    const struct dmxbox_serializer_entry *,
    const void *object
);

typedef struct dmxbox_serializer_entry {
  size_t offset;
  dmxbox_serialize_func_t serialize;
  const void *serialize_ctx;
  const char *json_name;
} dmxbox_serializer_entry_t;

static void *at_offset(const void *ptr, size_t offset) {
  return (uint8_t *)ptr + offset;
}

cJSON *dmxbox_serialize_u8(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  ESP_LOGI(TAG, "serializing u8 at offset %u", entry->offset);
  const uint8_t *ptr = (uint8_t *)at_offset(object, entry->offset);
  return cJSON_CreateNumber(*ptr);
}

cJSON *dmxbox_serialize_u16(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  ESP_LOGI(TAG, "serializing u16 at offset %u", entry->offset);
  const uint16_t *ptr = (uint16_t *)at_offset(object, entry->offset);
  return cJSON_CreateNumber(*ptr);
}

cJSON *dmxbox_serialize_ptr(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  ESP_LOGI(TAG, "serializing ptr at offset %u", entry->offset);
  const void *ptr = at_offset(object, entry->offset);
  dmxbox_ptr_to_json_func_t func = entry->serialize_ctx;
  return func(ptr);
}

cJSON *dmxbox_serialize_object(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  cJSON *json = cJSON_CreateObject();
  if (!json) {
    return NULL;
  }
  while (entry->json_name) {
    ESP_LOGI(TAG, "serializing %s", entry->json_name);
    cJSON *item = entry->serialize(entry, object);
    if (!item) {
      goto error;
    }
    if (!cJSON_AddItemToObjectCS(json, entry->json_name, item)) {
      cJSON_free(item);
      goto error;
    }
    entry++;
  }

  return json;
error:
  cJSON_free(json);
  return NULL;
}

#define BEGIN_DMXBOX_API_SERIALIZER(name)                                      \
  static const dmxbox_serializer_entry_t name[] = {

#define DMXBOX_API_SERIALIZE_U8(type, name)                                    \
  {.serialize = dmxbox_serialize_u8,                                           \
   .json_name = #name,                                                         \
   .offset = offsetof(type, name)},

#define DMXBOX_API_SERIALIZE_U16(type, name)                                   \
  {.serialize = dmxbox_serialize_u16,                                          \
   .json_name = #name,                                                         \
   .offset = offsetof(type, name)},

#define DMXBOX_API_SERIALIZE_PTR(type, name, to_json_fn)                       \
  {.serialize = dmxbox_serialize_ptr,                                          \
   .serialize_ctx = to_json_fn,                                                \
   .json_name = #name,                                                         \
   .offset = offsetof(type, name)},

#define END_DMXBOX_API_SERIALIZER()                                            \
  { .json_name = NULL }                                                        \
  }                                                                            \
  ;

BEGIN_DMXBOX_API_SERIALIZER(channel_level_serializer)
DMXBOX_API_SERIALIZE_PTR(
    dmxbox_storage_channel_level_t,
    channel,
    dmxbox_api_channel_to_json
)
DMXBOX_API_SERIALIZE_U8(dmxbox_storage_channel_level_t, level)
END_DMXBOX_API_SERIALIZER()

BEGIN_DMXBOX_API_SERIALIZER(effect_step_header_serializer)
DMXBOX_API_SERIALIZE_U16(dmxbox_storage_effect_step_t, time)
DMXBOX_API_SERIALIZE_U16(dmxbox_storage_effect_step_t, in)
DMXBOX_API_SERIALIZE_U16(dmxbox_storage_effect_step_t, dwell)
DMXBOX_API_SERIALIZE_U16(dmxbox_storage_effect_step_t, out)
END_DMXBOX_API_SERIALIZER()

static cJSON *dmxbox_api_effect_step_to_json(
    const dmxbox_storage_effect_step_t *object,
    size_t channel_count
) {
  cJSON *json = dmxbox_serialize_object(effect_step_header_serializer, object);

  cJSON *channels = cJSON_AddArrayToObject(json, "channels");
  if (!channels) {
    goto exit;
  }

  for (size_t i = 0; i < channel_count; i++) {
    cJSON *channel_level =
        dmxbox_serialize_object(channel_level_serializer, &object->channels[i]);
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
