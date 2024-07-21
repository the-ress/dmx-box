#include <cJSON.h>
#include <esp_check.h>
#include <esp_err.h>
#include <esp_http_server.h>

#include "api_strings.h"
#include "dmxbox_httpd.h"
#include "settings_artnet.h"

static const char TAG[] = "dmxbox_api_settings_artnet";

static const char field_native_universe[] = "native_universe";
static const char field_effect_control_universe[] = "effect_control_universe";

static esp_err_t dmxbox_api_settings_artnet_get(httpd_req_t *req) {
  ESP_LOGI(TAG, "GET request for %s", req->uri);

  dmxbox_httpd_cors_allow_origin(req);

  esp_err_t ret = ESP_ERR_NO_MEM;
  cJSON *json = cJSON_CreateObject();
  if (!json) {
    goto exit;
  }

  uint16_t native_universe = dmxbox_get_native_universe();
  uint16_t effect_control_universe = dmxbox_get_effect_control_universe();
  if (!cJSON_AddNumberToObject(json, field_native_universe, native_universe)) {
    goto exit;
  }
  if (!cJSON_AddNumberToObject(
          json,
          field_effect_control_universe,
          effect_control_universe
      )) {
    goto exit;
  }
  ret = dmxbox_httpd_send_json(req, json);
exit:
  if (json) {
    cJSON_free(json);
  }
  return ret;
}

static esp_err_t dmxbox_api_settings_artnet_put(httpd_req_t *req) {
  ESP_LOGI(TAG, "PUT request for %s", req->uri);

  dmxbox_httpd_cors_allow_origin(req);

  esp_err_t ret = ESP_OK;
  const char *http_status = HTTPD_400;
  cJSON *json = NULL;

  ESP_RETURN_ON_ERROR(
      dmxbox_httpd_receive_json(req, &json),
      TAG,
      "failed to receive json"
  );

  cJSON *native_universe =
      cJSON_GetObjectItemCaseSensitive(json, field_native_universe);
  if (!native_universe || !cJSON_IsNumber(native_universe)) {
    ESP_LOGE(TAG, "native_universe missing or not a number");
    goto send;
  }

  cJSON *effect_control_universe =
      cJSON_GetObjectItemCaseSensitive(json, field_effect_control_universe);
  if (!effect_control_universe || !cJSON_IsNumber(effect_control_universe)) {
    ESP_LOGE(TAG, "effect_control_universe missing or not a number");
    goto send;
  }

  dmxbox_set_native_universe(native_universe->valueint);
  dmxbox_set_effect_control_universe(effect_control_universe->valueint);

  http_status = HTTPD_204;

send:

  ESP_GOTO_ON_ERROR(
      httpd_resp_set_status(req, http_status),
      exit,
      TAG,
      "failed to set status"
  );

  ESP_GOTO_ON_ERROR(
      httpd_resp_send_chunk(req, NULL, 0),
      exit,
      TAG,
      "failed to send empty chunk"
  );
exit:
  cJSON_free(json);
  return ret;
}

esp_err_t dmxbox_api_settings_artnet_register(httpd_handle_t server) {
  static const httpd_uri_t get = {
      .uri = "/api/settings/artnet",
      .method = HTTP_GET,
      .handler = dmxbox_api_settings_artnet_get,
  };
  static const httpd_uri_t put = {
      .uri = "/api/settings/artnet",
      .method = HTTP_PUT,
      .handler = dmxbox_api_settings_artnet_put,
  };
  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &get),
      TAG,
      "handler_config_get failed"
  );
  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &put),
      TAG,
      "handler_config_put failed"
  );
  return ESP_OK;
}
