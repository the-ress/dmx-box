#include "effects.h"
#include "dmxbox_httpd.h"
#include "dmxbox_uri.h"
#include "effects_steps.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "http_parser.h"
#include <cJSON.h>
#include <esp_log.h>
#include <inttypes.h>
#include <string.h>

static const char TAG[] = "dmxbox_api_effect";

static esp_err_t dmxbox_api_effect_parse_uri(
    httpd_req_t *req,
    uint16_t *effect_id,
    uint16_t *step_id
) {
  if (!effect_id || !step_id) {
    return ESP_ERR_INVALID_ARG;
  }
  *effect_id = 0;
  *step_id = 0;

  const char *uri = req->uri;
  uri = dmxbox_uri_match_component("api", uri);
  uri = dmxbox_uri_match_component("effects", uri);

  if (!uri) {
    return ESP_ERR_NOT_FOUND;
  }
  if (*uri == '\0') {
    ESP_LOGI(TAG, "uri=effect container");
    return ESP_OK;
  }

  uri = dmxbox_uri_match_u16(effect_id, uri);
  if (!uri) {
    ESP_LOGW(TAG, "uri effect id not u16");
    return ESP_ERR_NOT_FOUND;
  }
  if (*uri == '\0') {
    ESP_LOGI(TAG, "uri effect id %u", *effect_id);
    return ESP_OK;
  }

  uri = dmxbox_uri_match_component("steps", uri);
  if (!uri) {
    ESP_LOGW(TAG, "uri bad effect child");
    return ESP_ERR_NOT_FOUND;
  }

  if (*uri == '\0') {
    ESP_LOGI(TAG, "uri step container");
    return ESP_OK;
  }

  uri = dmxbox_uri_match_u16(step_id, uri);
  if (!uri) {
    ESP_LOGW(TAG, "uri step id not u16");
    return ESP_ERR_NOT_FOUND;
  }
  if (*uri == '\0') {
    ESP_LOGI(TAG, "uri step id %u", *step_id);
    return ESP_OK;
  }

  ESP_LOGW(TAG, "uri step trailing segments");
  return ESP_ERR_NOT_FOUND;
}

esp_err_t dmxbox_api_effects_endpoint(httpd_req_t *req) {
  ESP_LOGI(TAG, "got '%s'", req->uri);

  uint16_t effect_id = 0;
  uint16_t step_id = 0;

  esp_err_t ret = dmxbox_api_effect_parse_uri(req, &effect_id, &step_id);
  switch (ret) {
  case ESP_OK:
    break;
  case ESP_ERR_NOT_FOUND:
    return httpd_resp_send_404(req);
  default:
    return ret;
  }

  if (!effect_id) {
    // effect list
    return dmxbox_httpd_send_jsonstr(req, "[]");
  } else if (!step_id) {
    // step list
    return dmxbox_httpd_send_jsonstr(req, "[]");
  } else {
    return dmxbox_api_effect_step_endpoint(req, effect_id, step_id);
  }
}

esp_err_t dmxbox_api_effects_register(httpd_handle_t server) {
  httpd_uri_t endpoint = {
      .uri = "/api/effects/*",
      .method = HTTP_GET,
      .handler = dmxbox_api_effects_endpoint,
  };
  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &endpoint),
      TAG,
      "failed to register GET endpoint"
  );
  endpoint.method = HTTP_PUT;
  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &endpoint),
      TAG,
      "failed to register PUT endpoint"
  );
  return ESP_OK;
}
