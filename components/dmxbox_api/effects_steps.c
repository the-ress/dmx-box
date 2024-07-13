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

static cJSON *dmxbox_api_channel_level_to_json(
    const dmxbox_storage_channel_level_t *channel_level
) {
  cJSON *json = cJSON_CreateObject();
  if (!json) {
    return NULL;
  }

  cJSON *channel = dmxbox_api_channel_to_json(channel_level->channel);
  if (!channel) {
    goto error;
  }

  if (!cJSON_AddItemToObjectCS(json, "channel", channel)) {
    cJSON_free(channel);
    goto error;
  }

  if (!cJSON_AddNumberToObject(json, "level", channel_level->level)) {
    goto error;
  }

  return json;

error:
  cJSON_free(json);
  return json;
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

  json = cJSON_CreateObject();
  if (!json) {
    ESP_LOGE(TAG, "failed to allocate json");
    goto exit;
  }

  if (!cJSON_AddNumberToObject(json, "time", effect_step->time)) {
    goto exit;
  }

  if (!cJSON_AddNumberToObject(json, "in", effect_step->in)) {
    goto exit;
  }

  if (!cJSON_AddNumberToObject(json, "dwell", effect_step->dwell)) {
    goto exit;
  }

  if (!cJSON_AddNumberToObject(json, "out", effect_step->out)) {
    goto exit;
  }

  cJSON *channels = cJSON_AddArrayToObject(json, "channels");
  if (!channels) {
    goto exit;
  }

  for (size_t i = 0; i < channel_count; i++) {
    cJSON *channel_level =
        dmxbox_api_channel_level_to_json(&effect_step->channels[i]);
    if (!cJSON_AddItemToArray(channels, channel_level)) {
      cJSON_free(channel_level);
      goto exit;
    }
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
