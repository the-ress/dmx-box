#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_wifi.h"
#include <string.h>

#include "storage.h"
#include "webserver.h"
#include "wifi.h"

static const char *TAG = "dmxbox_webserver";

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

#define CHECK_FILE_EXTENSION(filename, ext)                                    \
  (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

static char scratch[SCRATCH_BUFSIZE];

/* Set HTTP response content type according to file extension */
static esp_err_t
set_content_type_from_file(httpd_req_t *req, const char *filepath) {
  const char *type = "text/plain";
  if (CHECK_FILE_EXTENSION(filepath, ".html")) {
    type = "text/html";
  } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
    type = "application/javascript";
  } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
    type = "text/css";
  } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
    type = "image/png";
  } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
    type = "image/x-icon";
  } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
    type = "text/xml";
  } else if (CHECK_FILE_EXTENSION(filepath, ".woff2")) {
    type = "font/woff2";
  }
  return httpd_resp_set_type(req, type);
}

static esp_err_t common_get_handler(httpd_req_t *req) {
  char filepath[FILE_PATH_MAX];

  strlcpy(filepath, WEB_MOUNT_POINT, sizeof(filepath));
  if (req->uri[strlen(req->uri) - 1] == '/') {
    strlcat(filepath, "/index.html", sizeof(filepath));
  } else {
    strlcat(filepath, req->uri, sizeof(filepath));
  }
  int fd = open(filepath, O_RDONLY, 0);
  if (fd == -1) {
    ESP_LOGE(TAG, "Failed to open file : %s", filepath);
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
    return ESP_FAIL;
  }

  set_content_type_from_file(req, filepath);

  // http_passthrough(req);

  char *chunk = scratch;
  ssize_t read_bytes;
  do {
    /* Read file in chunks into the scratch buffer */
    read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
    if (read_bytes == -1) {
      ESP_LOGE(TAG, "Failed to read file : %s", filepath);
    } else if (read_bytes > 0) {
      ESP_LOGD(TAG, "Sending %d-byte chunk of %s", read_bytes, filepath);
      /* Send the buffer contents as HTTP response chunk */
      if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
        close(fd);
        ESP_LOGE(TAG, "File sending failed: %s", filepath);
        /* Abort sending file */
        httpd_resp_sendstr_chunk(req, NULL);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(
            req,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "Failed to send file"
        );
        return ESP_FAIL;
      }
    }
  } while (read_bytes > 0);
  /* Close file after sending complete */
  close(fd);
  ESP_LOGI(TAG, "File sending complete: %s", filepath);
  /* Respond with an empty chunk to signal HTTP response completion */
  httpd_resp_send_chunk(req, NULL, 0);
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

  bool sta_mode_enabled = get_sta_mode_enabled();

  cJSON *ap, *sta;
  cJSON *root = cJSON_CreateObject();
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
  int total_len = req->content_len;
  int cur_len = 0;
  int received = 0;
  if (total_len >= SCRATCH_BUFSIZE) {
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(
        req,
        HTTPD_500_INTERNAL_SERVER_ERROR,
        "Content too long"
    );
    return ESP_FAIL;
  }
  while (cur_len < total_len) {
    received = httpd_req_recv(req, scratch + cur_len, total_len);
    if (received <= 0) {
      /* Respond with 500 Internal Server Error */
      httpd_resp_send_err(
          req,
          HTTPD_500_INTERNAL_SERVER_ERROR,
          "Failed to save config"
      );
      return ESP_FAIL;
    }
    cur_len += received;
  }
  scratch[total_len] = '\0';

  cJSON *root = cJSON_Parse(scratch);
  cJSON *ap = cJSON_GetObjectItem(root, "ap");
  cJSON *sta = cJSON_GetObjectItem(root, "sta");

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

  cJSON_Delete(root);
  httpd_resp_sendstr(req, "Config was stored successfully");
  return ESP_OK;
}

static const httpd_uri_t api_wifi_config_get = {
    .uri = "/api/wifi-config",
    .method = HTTP_GET,
    .handler = api_wifi_config_get_handler,
};

static const httpd_uri_t api_wifi_config_put = {
    .uri = "/api/wifi-config",
    .method = HTTP_PUT,
    .handler = api_wifi_config_put_handler,
};

/* URI handler for getting web server files */
static const httpd_uri_t common_get_uri = {
    .uri = "/*",
    .method = HTTP_GET,
    .handler = common_get_handler,
};

void start_webserver(void) {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.task_priority = 1;
  config.max_open_sockets = 10;
  config.lru_purge_enable = true;
  config.uri_match_fn = httpd_uri_match_wildcard;

  // Start the httpd server
  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if (!httpd_start(&server, &config) == ESP_OK) {
    ESP_LOGI(TAG, "Error starting server!");
    return;
  }

  // Set URI handlers
  ESP_LOGI(TAG, "Registering URI handlers");
  httpd_register_uri_handler(server, &api_wifi_config_get);
  httpd_register_uri_handler(server, &api_wifi_config_put);
  httpd_register_uri_handler(server, &common_get_uri);
}
