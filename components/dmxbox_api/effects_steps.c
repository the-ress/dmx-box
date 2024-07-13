#include "cJSON.h"
#include "dmxbox_httpd.h"
#include "dmxbox_storage.h"
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

BEGIN_DMXBOX_API_SERIALIZER(dmxbox_storage_channel_level_t, channel_level)
DMXBOX_API_SERIALIZE_SUB_OBJECT(
    dmxbox_storage_channel_level_t,
    channel,
    dmxbox_api_channel_to_json
)
DMXBOX_API_SERIALIZE_U8(dmxbox_storage_channel_level_t, level)
END_DMXBOX_API_SERIALIZER(dmxbox_storage_channel_level_t, channel_level)

BEGIN_DMXBOX_API_SERIALIZER(dmxbox_storage_effect_step_t, effect_step)
DMXBOX_API_SERIALIZE_U16(dmxbox_storage_effect_step_t, time)
DMXBOX_API_SERIALIZE_U16(dmxbox_storage_effect_step_t, in)
DMXBOX_API_SERIALIZE_U16(dmxbox_storage_effect_step_t, dwell)
DMXBOX_API_SERIALIZE_U16(dmxbox_storage_effect_step_t, out)
DMXBOX_API_SERIALIZE_TRAILING_ARRAY(
    dmxbox_storage_effect_step_t,
    channels,
    channel_count,
    dmxbox_channel_level_to_json
)
END_DMXBOX_API_SERIALIZER(dmxbox_storage_effect_step_t, effect_step)

esp_err_t dmxbox_api_effect_step_get(
    httpd_req_t *req,
    uint16_t effect_id,
    uint16_t step_id
) {
  dmxbox_storage_effect_step_t *effect_step = NULL;
  cJSON *json = NULL;

  esp_err_t ret = dmxbox_storage_effect_step_get(
      effect_id,
      step_id,
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

  json = dmxbox_effect_step_to_json(effect_step);
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
