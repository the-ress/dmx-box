#include "settings_sta.h"
#include "cJSON.h"
#include "esp_err.h"
#include "esp_wifi_types.h"
#include "private/api_strings.h"
#include "private/receive_json.h"
#include "wifi.h"
#include <esp_check.h>
#include <esp_http_server.h>

static const char TAG[] = "dmxbox_api_settings_sta";

static const char field_enabled[] = "enabled";
static const char field_ssid[] = "ssid";

static esp_err_t dmxbox_api_settings_sta_get(httpd_req_t *req) {
  esp_err_t ret = ESP_ERR_NO_MEM;
  cJSON *json = cJSON_CreateObject();
  if (!json) {
    goto exit;
  }
  dmxbox_wifi_sta_t sta;
  ESP_GOTO_ON_ERROR(dmxbox_wifi_get_sta(&sta), exit, TAG,
                    "failed to get sta information");
  if (!cJSON_AddBoolToObject(json, field_enabled, sta.enabled)) {
    goto exit;
  }
  if (!cJSON_AddStringToObject(json, field_ssid, sta.ssid)) {
    goto exit;
  }
  ret = dmxbox_httpd_send_json(req, json);
exit:
  if (json) {
    cJSON_free(json);
  }
  return ret;
}

static esp_err_t dmxbox_api_settings_sta_put(httpd_req_t *req) {
  esp_err_t ret = ESP_OK;
  const char *http_status = HTTPD_400;
  cJSON *json = NULL;

  ESP_RETURN_ON_ERROR(dmxbox_httpd_receive_json(req, &json), TAG,
                      "failed to receive json");

  cJSON *enabled = cJSON_GetObjectItemCaseSensitive(json, field_enabled);
  if (!enabled || !cJSON_IsBool(enabled)) {
    ESP_LOGE(TAG, "enabled missing or not a bool");
    goto send;
  }

  if (cJSON_IsTrue(enabled)) {
    cJSON *ssid = cJSON_GetObjectItemCaseSensitive(json, field_ssid);
    if (!ssid || !cJSON_IsString(ssid)) {
      ESP_LOGE(TAG, "ssid missing or not a string");
      goto send;
    }

    cJSON *auth_mode_str = cJSON_GetObjectItemCaseSensitive(json, "authMode");
    if (!auth_mode_str || !cJSON_IsString(auth_mode_str)) {
      ESP_LOGE(TAG, "authMode missing or not a string");
      goto send;
    }

    wifi_auth_mode_t auth_mode;
    if (!dmxbox_auth_mode_from_str(auth_mode_str->valuestring, &auth_mode)) {
      ESP_LOGE(TAG, "invalid authMode");
      goto send;
    }

    cJSON *password = cJSON_GetObjectItemCaseSensitive(json, "password");
    if (auth_mode == WIFI_AUTH_OPEN && password) {
      ESP_LOGE(TAG, "password not used with open auth");
      goto send;
    } else if (!password || !cJSON_IsString(password)) {
      ESP_LOGE(TAG, "password missing or not a string");
      goto send;
    }

    if (dmxbox_wifi_enable_sta(ssid->valuestring, auth_mode,
                               password->valuestring) != ESP_OK) {
      goto send;
    }
  } else if (dmxbox_wifi_disable_sta() != ESP_OK) {
    http_status = HTTPD_500;
    goto send;
  }

  http_status = HTTPD_204;

send:

  ESP_GOTO_ON_ERROR(httpd_resp_set_status(req, http_status), exit, TAG,
                    "failed to set status");

  ESP_GOTO_ON_ERROR(httpd_resp_send_chunk(req, NULL, 0), exit, TAG,
                    "failed to send empty chunk");
exit:
  cJSON_free(json);
  return ret;
}

esp_err_t dmxbox_api_settings_sta_register(httpd_handle_t server) {
  static const httpd_uri_t get = {
      .uri = "/api/settings/sta",
      .method = HTTP_GET,
      .handler = dmxbox_api_settings_sta_get,
  };
  static const httpd_uri_t put = {
      .uri = "/api/settings/sta",
      .method = HTTP_PUT,
      .handler = dmxbox_api_settings_sta_put,
  };
  ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &get), TAG,
                      "handler_config_get failed");
  ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &put), TAG,
                      "handler_config_put failed");
  return ESP_OK;
}
