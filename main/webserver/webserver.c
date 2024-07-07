#include <cJSON.h>
#include <esp_check.h>
#include <esp_http_client.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <string.h>

#include "static_files.h"
#include "storage.h"
#include "webserver.h"
#include "wifi.h"
#include "wifi_scan.h"

static const char *TAG = "dmxbox_webserver";

#define CONFIG_CORS_ORIGIN "http://localhost:8000"
#ifdef CONFIG_CORS_ORIGIN
static const char *dmxbox_api_cors_origin = CONFIG_CORS_ORIGIN;
#else
static const char *dmxbox_api_cors_origin = NULL;
#endif

/* Set HTTP response content type according to file extension */
static esp_err_t dmxbox_api_set_cors_header(httpd_req_t *req) {
  if (dmxbox_api_cors_origin) {
    ESP_RETURN_ON_ERROR(
        httpd_resp_set_hdr(
            req,
            "Access-Control-Allow-Origin",
            dmxbox_api_cors_origin
        ),
        __func__,
        "failed to set allow origin header"
    );
    ESP_RETURN_ON_ERROR(
        httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET,PUT"),
        __func__,
        "failed to set allow methods header"
    );
  }
  return ESP_OK;
}

static const char *auth_mode_to_string(wifi_auth_mode_t auth_mode) {
  static const char *strings[] = {
      "open",
      "WEP",
      "WPA_PSK",
      "WPA2_PSK",
      "WPA_WPA2_PSK",
      "", // WPA2_ENTERPRISE
      "WPA3_PSK",
      "WPA2_WPA3_PSK",
  };
  if (auth_mode >= 0 && auth_mode < (sizeof(strings) / sizeof(strings[0]))) {
    return strings[auth_mode];
  }
  ESP_LOGE(TAG, "Unknown auth mode: %d", auth_mode);
  return "";
}

static wifi_auth_mode_t string_to_auth_mode(char *str) {
  if (strcmp(str, "open") == 0) {
    return WIFI_AUTH_OPEN;
  } else if (strcmp(str, "WEP") == 0) {
    return WIFI_AUTH_WEP;
  } else if (strcmp(str, "WPA_PSK") == 0) {
    return WIFI_AUTH_WPA_PSK;
  } else if (strcmp(str, "WPA2_PSK") == 0) {
    return WIFI_AUTH_WPA2_PSK;
  } else if (strcmp(str, "WPA_WPA2_PSK") == 0) {
    return WIFI_AUTH_WPA_WPA2_PSK;
  } else if (strcmp(str, "WPA2_ENTERPRISE") == 0) {
    return WIFI_AUTH_WPA2_ENTERPRISE;
  } else if (strcmp(str, "WPA3_PSK") == 0) {
    return WIFI_AUTH_WPA3_PSK;
  } else if (strcmp(str, "WPA2_WPA3_PSK") == 0) {
    return WIFI_AUTH_WPA2_WPA3_PSK;
  } else if (strcmp(str, "WAPI_PSK") == 0) {
    return WIFI_AUTH_WAPI_PSK;
  } else {
    ESP_LOGE(TAG, "Unknown auth mode string: %s", str);
    return WIFI_AUTH_WPA_WPA2_PSK;
  }
}

static esp_err_t api_wifi_config_get_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "application/json");
  dmxbox_api_set_cors_header(req);

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
      auth_mode_to_string(dmxbox_wifi_config.ap.auth_mode)
  );
  cJSON_AddNumberToObject(ap, "channel", dmxbox_wifi_config.ap.channel);

  cJSON_AddBoolToObject(sta, "enabled", sta_mode_enabled);

  cJSON_AddStringToObject(sta, "ssid", dmxbox_wifi_config.sta.ssid);
  cJSON_AddStringToObject(sta, "password", dmxbox_wifi_config.sta.password);
  cJSON_AddStringToObject(
      sta,
      "auth_mode",
      auth_mode_to_string(dmxbox_wifi_config.sta.auth_mode)
  );

  const char *result = cJSON_Print(root);
  httpd_resp_sendstr(req, result);
  free((void *)result);
  cJSON_Delete(root);
  return ESP_OK;
}

static esp_err_t api_wifi_config_put_handler(httpd_req_t *req) {
  static char scratch[10240];

  int total_len = req->content_len;
  int cur_len = 0;
  int received = 0;
  if (total_len >= sizeof(scratch)) {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Content too long");
    return ESP_OK;
  }
  while (cur_len < total_len) {
    received = httpd_req_recv(req, scratch + cur_len, total_len);
    if (received <= 0) {
      httpd_resp_send_err(
          req,
          HTTPD_408_REQ_TIMEOUT,
          "Failed to receive request"
      );
      return ESP_OK;
    }
    cur_len += received;
  }
  scratch[total_len] = '\0';

  cJSON *root = cJSON_Parse(scratch);
  cJSON *ap = cJSON_GetObjectItem(root, "ap");
  cJSON *sta = cJSON_GetObjectItem(root, "sta");

  const char *hostname = cJSON_GetObjectItem(root, "hostname")->valuestring;

  char *ap_ssid = cJSON_GetObjectItem(ap, "ssid")->valuestring;
  char *ap_password = cJSON_GetObjectItem(ap, "password")->valuestring;
  char *ap_auth_mode_string = cJSON_GetObjectItem(ap, "auth_mode")->valuestring;
  wifi_auth_mode_t ap_auth_mode = string_to_auth_mode(ap_auth_mode_string);
  uint8_t ap_channel = (uint8_t)cJSON_GetObjectItem(ap, "channel")->valueint;

  bool sta_mode_enabled = cJSON_IsTrue(cJSON_GetObjectItem(sta, "enabled"));
  char *sta_ssid = cJSON_GetObjectItem(sta, "ssid")->valuestring;
  char *sta_password = cJSON_GetObjectItem(sta, "password")->valuestring;
  char *sta_auth_mode_string =
      cJSON_GetObjectItem(sta, "auth_mode")->valuestring;
  wifi_auth_mode_t sta_auth_mode = string_to_auth_mode(sta_auth_mode_string);

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

static esp_err_t dmxbox_api_options_handler(httpd_req_t *req) {
  ESP_LOGD(TAG, "OPTIONS request for %s", req->uri);
  ESP_RETURN_ON_ERROR(
      dmxbox_api_set_cors_header(req),
      __func__,
      "set_cors_header fail"
  );
  ESP_RETURN_ON_ERROR(httpd_resp_send(req, "", 0), __func__, "resp_send fail");
  return ESP_OK;
}

esp_err_t dmxbox_start_webserver(void) {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.task_priority = 1;
  config.max_open_sockets = 10;
  config.lru_purge_enable = true;
  config.uri_match_fn = httpd_uri_match_wildcard;

  // Start the httpd server
  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  ESP_RETURN_ON_ERROR(
      httpd_start(&server, &config),
      __func__,
      "httpd_start failed"
  );

  static const httpd_uri_t handler_config_get = {
      .uri = "/api/wifi-config",
      .method = HTTP_GET,
      .handler = api_wifi_config_get_handler,
  };

  static const httpd_uri_t handler_config_options = {
      .uri = "/api/wifi-config",
      .method = HTTP_OPTIONS,
      .handler = dmxbox_api_options_handler,
  };

  static const httpd_uri_t handler_config_put = {
      .uri = "/api/wifi-config",
      .method = HTTP_PUT,
      .handler = api_wifi_config_put_handler,
  };

  static const httpd_uri_t handler_wifi_scan = {
      .uri = "/api/wifi-scan",
      .method = HTTP_GET,
      .handler = dmxbox_httpd_wifi_scan_handler,
      .is_websocket = true,
  };

  static const httpd_uri_t handler_wildcard = {
      .uri = "/*",
      .method = HTTP_GET,
      .handler = &dmxbox_httpd_static_handler,
  };

  ESP_LOGI(TAG, "Registering URI handlers");

  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &handler_config_get),
      __func__,
      "handler_config_get failed"
  );

  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &handler_config_put),
      __func__,
      "handler_config_put failed"
  );

  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &handler_config_options),
      __func__,
      "handler_config_options failed"
  );

  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &handler_wifi_scan),
      __func__,
      "handler_wifi_scan failed"
  );

  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &handler_wildcard),
      __func__,
      "handler_wildcard failed"
  );

  return ESP_OK;
}
