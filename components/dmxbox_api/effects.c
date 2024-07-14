#include "effects.h"
#include "dmxbox_httpd.h"
#include "dmxbox_storage.h"
#include "effect_storage.h"
#include "esp_check.h"
#include "esp_http_server.h"
#include "serializer.h"
#include <esp_log.h>

static const char TAG[] = "dmxbox_api_effect";

BEGIN_DMXBOX_API_SERIALIZER(dmxbox_effect_t, effect)
DMXBOX_API_SERIALIZE_U16(dmxbox_effect_t, level_channel)
DMXBOX_API_SERIALIZE_U16(dmxbox_effect_t, rate_channel)
DMXBOX_API_SERIALIZE_U32(dmxbox_effect_t, step_count)
END_DMXBOX_API_SERIALIZER(dmxbox_effect_t, effect)

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

esp_err_t dmxbox_api_effect_endpoint(httpd_req_t *req, uint16_t effect_id) {
  if (req->method == HTTP_GET) {
    return dmxbox_api_effect_get(req, effect_id);
  } else if (req->method == HTTP_PUT) {
    // return dmxbox_api_effect_put(req, effect_id);
  }
  return httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED, NULL);
}
