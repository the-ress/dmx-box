#include "router_effects.h"
#include "dmxbox_httpd.h"
#include "dmxbox_rest.h"
#include "effects.h"
#include "effects_steps.h"
#include <esp_check.h>

static const char TAG[] = "dmxbox_api_router_effects";

static const dmxbox_rest_child_container_t router_steps = {
    .sibling = NULL,
    .container = {.slug = "steps"},
};

static const dmxbox_rest_container_t router = {
    .slug = "effects",
    .first_child = &router_steps};

static esp_err_t dmxbox_api_effects_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "got '%s'", req->uri);

  dmxbox_rest_uri_t uri;
  esp_err_t ret = dmxbox_rest_parse_uri(&router, req, &uri);
  switch (ret) {
  case ESP_OK:
    break;
  case ESP_ERR_NOT_FOUND:
    return httpd_resp_send_404(req);
  default:
    return ret;
  }

  if (uri.parent_id) {
    if (uri.child_id) {
      return dmxbox_api_effect_step_endpoint(req, uri.parent_id, uri.child_id);
    } else {
      return dmxbox_api_effect_endpoint(req, uri.parent_id);
    }
  } else {
    return dmxbox_api_effect_container_endpoint(req);
  }
}

esp_err_t dmxbox_api_effects_register(httpd_handle_t server) {
  httpd_uri_t endpoint = {
      .uri = "/api/effects/*",
      .method = HTTP_GET,
      .handler = dmxbox_api_effects_handler,
  };
  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &endpoint),
      TAG,
      "failed to register GET endpoint"
  );
  endpoint.method = HTTP_POST;
  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &endpoint),
      TAG,
      "failed to register POST endpoint"
  );
  endpoint.method = HTTP_PUT;
  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &endpoint),
      TAG,
      "failed to register PUT endpoint"
  );
  return ESP_OK;
}
