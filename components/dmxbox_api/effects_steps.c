#include "effects_steps.h"
#include "esp_check.h"
#include "esp_http_server.h"
#include "http_parser.h"
#include <esp_log.h>
#include <inttypes.h>
#include <string.h>

static const char TAG[] = "dmxbox_api_effect_steps";

static const char *dmxbox_uri_match_int(int *result, const char *uri) {
  if (!uri) {
    return NULL;
  }
  int value = 0;
  if (*uri == '/') {
    uri++;
  }
  for (; *uri >= '0' && *uri <= '9'; uri++) {
    value *= 10;
    value += (*uri - '0');
  }
  if (*uri == '/' || *uri == '\0') {
    *result = value;
    return uri;
  }
  *result = 0;
  return NULL;
}

static const char *
dmxbox_uri_match_component(const char *expected, const char *uri) {
  if (!uri) {
    return NULL;
  }
  if (*uri == '/') {
    uri++;
  }
  for (; *uri && *expected; uri++, expected++) {
    if (*uri != *expected) {
      return NULL;
    }
  }
  if (*uri == *expected) {
    return (*uri == '/') || (*uri == '\0') ? uri : NULL;
  }
  return NULL;
}

esp_err_t dmxbox_api_effect_steps_list(httpd_req_t *req) {
  const char *uri = req->uri;
  int effect_id = 0;
  uri = dmxbox_uri_match_component("api", uri);
  uri = dmxbox_uri_match_component("effects", uri);
  uri = dmxbox_uri_match_int(&effect_id, uri);
  uri = dmxbox_uri_match_component("steps", uri);
  if (uri && *uri == '\0') {
    ESP_LOGI(TAG, "matched effect_id=%d", effect_id);
    return httpd_resp_sendstr(req, "200 Okay I guess");
  } else {
    return httpd_resp_send_404(req);
  }
}

esp_err_t dmxbox_api_effect_steps_register(httpd_handle_t server) {
  static const httpd_uri_t list = {
      .uri = "/api/effects/*",
      .method = HTTP_GET,
      .handler = dmxbox_api_effect_steps_list,
  };
  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &list),
      TAG,
      "failed to register list endpoint"
  );
  return ESP_OK;
}
