#include "effects.h"
#include "dmxbox_httpd.h"
#include "dmxbox_uri.h"
#include "effects_steps.h"
#include "esp_check.h"
#include "esp_http_server.h"
#include "http_parser.h"
#include <cJSON.h>
#include <esp_log.h>
#include <inttypes.h>
#include <string.h>

static const char TAG[] = "dmxbox_api_effect";

esp_err_t dmxbox_api_effects_get(httpd_req_t *req) {
  ESP_LOGI(TAG, "GET %s", req->uri);

  const char *uri = req->uri;
  uint16_t effect_id = 0;
  uint16_t step_id = 0;
  uri = dmxbox_uri_match_component("api", uri);
  uri = dmxbox_uri_match_component("effects", uri);

  if (*uri == '\0') {
    // TODO list effects
    return dmxbox_httpd_send_jsonstr(req, "[]");
  }

  uri = dmxbox_uri_match_u16(&effect_id, uri);
  if (effect_id) {
    ESP_LOGE(TAG, "effect_id=%u", effect_id);
  } else {
    ESP_LOGE(TAG, "effect_id=0, not found");
    return httpd_resp_send_404(req);
  }

  uri = dmxbox_uri_match_component("steps", uri);
  if (!uri) {
    ESP_LOGE(TAG, "not found");
    return httpd_resp_send_404(req);
  }

  if (*uri == '\0') {
    // TODO list steps
    return dmxbox_httpd_send_jsonstr(req, "[]");
  }

  uri = dmxbox_uri_match_u16(&step_id, uri);
  if (uri && *uri == '\0' && step_id) {
    ESP_LOGI(TAG, "step_id=%u", step_id);
    return dmxbox_api_effect_step_get(req, effect_id, step_id);
  }

  ESP_LOGE(TAG, "not found");
  return httpd_resp_send_404(req);
}

esp_err_t dmxbox_api_effects_register(httpd_handle_t server) {
  static const httpd_uri_t list = {
      .uri = "/api/effects/*",
      .method = HTTP_GET,
      .handler = dmxbox_api_effects_get,
  };
  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &list),
      TAG,
      "failed to register list endpoint"
  );
  return ESP_OK;
}