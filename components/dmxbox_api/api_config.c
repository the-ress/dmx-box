#include "api_config.h"
#include "api_strings.h"
#include "dmxbox_httpd.h"
#include "dmxbox_storage.h"
#include "wifi.h"
#include <cJSON.h>
#include <esp_check.h>
#include <esp_log.h>

static const char TAG[] = "dmxbox_api_config";

static esp_err_t dmxbox_api_config_get(httpd_req_t *req) {
  ESP_LOGI(TAG, "GET request for %s", req->uri);
  ESP_RETURN_ON_ERROR(
      httpd_resp_set_type(req, "application/json"),
      TAG,
      "failed to set response content type"
  );
  dmxbox_httpd_cors_allow_origin(req);

  bool sta_mode_enabled = dmxbox_get_sta_mode_enabled();

  cJSON *ap, *sta;
  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "hostname", dmxbox_get_hostname());
  cJSON_AddItemToObject(root, "ap", ap = cJSON_CreateObject());
  cJSON_AddItemToObject(root, "sta", sta = cJSON_CreateObject());

  cJSON_AddStringToObject(ap, "ssid", dmxbox_wifi_config.ap.ssid);
  cJSON_AddStringToObject(ap, "password", dmxbox_wifi_config.ap.password);
  cJSON_AddStringToObject(
      ap,
      "auth_mode",
      dmxbox_auth_mode_to_str(dmxbox_wifi_config.ap.auth_mode)
  );
  cJSON_AddNumberToObject(ap, "channel", dmxbox_wifi_config.ap.channel);

  cJSON_AddBoolToObject(sta, "enabled", sta_mode_enabled);

  cJSON_AddStringToObject(sta, "ssid", dmxbox_wifi_config.sta.ssid);
  cJSON_AddStringToObject(sta, "password", dmxbox_wifi_config.sta.password);
  cJSON_AddStringToObject(
      sta,
      "auth_mode",
      dmxbox_auth_mode_to_str(dmxbox_wifi_config.sta.auth_mode)
  );

  const char *result = cJSON_Print(root);
  httpd_resp_sendstr(req, result);
  free((void *)result);
  cJSON_Delete(root);
  return ESP_OK;
}

static esp_err_t dmxbox_api_config_put(httpd_req_t *req) {
  ESP_LOGI(TAG, "PUT request for %s", req->uri);

  cJSON *root = NULL;
  ESP_RETURN_ON_ERROR(
      dmxbox_httpd_receive_json(req, &root),
      TAG,
      "failed to receive json request"
  );

  cJSON *ap = cJSON_GetObjectItem(root, "ap");
  cJSON *sta = cJSON_GetObjectItem(root, "sta");

  const char *hostname = cJSON_GetObjectItem(root, "hostname")->valuestring;

  char *ap_ssid = cJSON_GetObjectItem(ap, "ssid")->valuestring;
  char *ap_password = cJSON_GetObjectItem(ap, "password")->valuestring;
  char *ap_auth_mode_string = cJSON_GetObjectItem(ap, "auth_mode")->valuestring;

  wifi_auth_mode_t ap_auth_mode;
  if (!dmxbox_auth_mode_from_str(ap_auth_mode_string, &ap_auth_mode)) {
    ESP_LOGE(TAG, "Could not parse ap auth mode: %s", ap_auth_mode_string);
    return ESP_FAIL;
  }
  uint8_t ap_channel = (uint8_t)cJSON_GetObjectItem(ap, "channel")->valueint;

  bool sta_mode_enabled = cJSON_IsTrue(cJSON_GetObjectItem(sta, "enabled"));
  char *sta_ssid = cJSON_GetObjectItem(sta, "ssid")->valuestring;
  char *sta_password = cJSON_GetObjectItem(sta, "password")->valuestring;
  char *sta_auth_mode_string =
      cJSON_GetObjectItem(sta, "auth_mode")->valuestring;

  wifi_auth_mode_t sta_auth_mode;
  if (!dmxbox_auth_mode_from_str(sta_auth_mode_string, &sta_auth_mode)) {
    ESP_LOGE(TAG, "Could not parse sta auth mode: %s", sta_auth_mode_string);
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Got ap hostname: '%s'", hostname);
  ESP_LOGI(
      TAG,
      "Got ap values: ssid = %s, pw = %s, auth_mode = %s (%d), channel = %d",
      ap_ssid,
      ap_password,
      ap_auth_mode_string,
      ap_auth_mode,
      ap_channel
  );
  ESP_LOGI(
      TAG,
      "Got sta values: enabled = %d, ssid = %s, pw = %s, auth_mode = %s (%d)",
      sta_mode_enabled,
      sta_ssid,
      sta_password,
      sta_auth_mode_string,
      sta_auth_mode
  );

  if (!dmxbox_set_hostname(hostname)) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Hostname too long");
    goto exit;
  }

  dmxbox_wifi_config_t new_config;
  strlcpy(new_config.ap.ssid, ap_ssid, sizeof(new_config.ap.ssid));
  strlcpy(new_config.ap.password, ap_password, sizeof(new_config.ap.password));
  new_config.ap.auth_mode = ap_auth_mode;
  new_config.ap.channel = ap_channel;

  strlcpy(new_config.sta.ssid, sta_ssid, sizeof(new_config.sta.ssid));
  strlcpy(
      new_config.sta.password,
      sta_password,
      sizeof(new_config.sta.password)
  );
  new_config.sta.auth_mode = sta_auth_mode;

  wifi_update_config(&new_config, sta_mode_enabled);

  httpd_resp_sendstr(req, "Config was stored successfully");

exit:
  cJSON_Delete(root);
  return ESP_OK;
}

esp_err_t dmxbox_api_config_register(httpd_handle_t server) {
  static const char *const endpoint = "/api/wifi-config";
  static const httpd_uri_t get = {
      .uri = endpoint,
      .method = HTTP_GET,
      .handler = dmxbox_api_config_get,
  };
  static const httpd_uri_t put = {
      .uri = endpoint,
      .method = HTTP_PUT,
      .handler = dmxbox_api_config_put,
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
