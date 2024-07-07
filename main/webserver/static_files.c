#include "static_files.h"
#include <esp_check.h>
#include <esp_vfs.h>
#include <string.h>
#include <sys/fcntl.h>

#ifndef CONFIG_WEB_MOUNT_POINT
#define CONFIG_WEB_MOUNT_POINT "/www"
#endif

static const char *dmxbox_httpd_default_type = "application/octet-stream";

const char *dmxbox_httpd_type_from_path(const char *path) {
  const char *ext = strrchr(path, '.');
  if (!ext) {
    return dmxbox_httpd_default_type;
  }
  if (!strcmp(ext, ".js") || !strcmp(ext, ".mjs")) {
    return "application/javascript";
  }
  if (!strcmp(ext, ".html")) {
    return "text/html";
  }
  if (!strcmp(ext, ".css")) {
    return "text/css";
  }
  if (!strcmp(ext, ".jpeg")) {
    return "image/jpeg";
  }
  if (!strcmp(ext, ".png")) {
    return "image/png";
  }
  if (!strcmp(ext, ".ico")) {
    return "image/x-icon";
  }
  if (!strcmp(ext, ".woff2")) {
    return "font/woff2";
  }
  return dmxbox_httpd_default_type;
}

// strips leading /, ensures path contains no further / and no ..
const char *dmxbox_httpd_validate_uri(const char *uri) {
  if (*uri == '/') {
    uri++;
  }
  if (*uri == '\0') {
    return "index.html";
  }
  if (strchr(uri, '/')) {
    return NULL;
  }
  if (strstr(uri, "..")) {
    return NULL;
  }
  return uri;
}

esp_err_t dmxbox_httpd_static_handler(httpd_req_t *req) {
  int fd = -1;
  esp_err_t ret = ESP_OK;
  httpd_err_code_t http_status = HTTPD_500_INTERNAL_SERVER_ERROR;

  ESP_LOGI(__func__, "Handling %s", req->uri);

  const char *uri = dmxbox_httpd_validate_uri(req->uri);
  if (uri == NULL) {
    ESP_LOGE(__func__, "invalid URL");
    http_status = HTTPD_404_NOT_FOUND;
    ret = ESP_FAIL;
    goto send_response;
  }

  char filepath[140] = CONFIG_WEB_MOUNT_POINT;
  if (strlcat(filepath, "/", sizeof(filepath)) >= sizeof(filepath)) {
    http_status = HTTPD_404_NOT_FOUND;
    ret = ESP_FAIL;
    goto send_response;
  }

  if (strlcat(filepath, uri, sizeof(filepath)) >= sizeof(filepath)) {
    http_status = HTTPD_404_NOT_FOUND;
    ret = ESP_FAIL;
    goto send_response;
  }

  fd = open(filepath, O_RDONLY, 0);
  if (fd == -1) {
    ESP_LOGE(__func__, "Failed to open file : %s", filepath);
    http_status = HTTPD_404_NOT_FOUND;
    ret = ESP_FAIL;
    goto send_response;
  }

  const char *type = dmxbox_httpd_type_from_path(filepath);
  ESP_LOGI(__func__, "MIME type: %s", type);
  ESP_GOTO_ON_ERROR(
      httpd_resp_set_type(req, type),
      exit,
      __func__,
      "httpd_resp_set_type failed"
  );

  do {
    static char chunk[16384];
    size_t read_bytes = read(fd, chunk, sizeof(chunk));
    switch (read_bytes) {
    case -1:
      ESP_LOGE(__func__, "Failed to read file %s", filepath);
      ret = ESP_FAIL;
      http_status = HTTPD_500_INTERNAL_SERVER_ERROR;
      goto send_response;

    case 0:
      ESP_LOGI(__func__, "File sending complete: %s", filepath);
      goto send_response;

    default:
      ESP_LOGD(__func__, "Sending %d-byte chunk of %s", read_bytes, filepath);
      ESP_GOTO_ON_ERROR(
          httpd_resp_send_chunk(req, chunk, read_bytes),
          exit,
          __func__,
          "http_resp_send_chunk failed"
      );
    }
  } while (ret == ESP_OK);

send_response:
  if (ret == ESP_OK) {
    ESP_GOTO_ON_ERROR(
        httpd_resp_send_chunk(req, NULL, 0),
        exit,
        __func__,
        "failed to send the final chunk"
    );
  } else {
    ESP_GOTO_ON_ERROR(
        httpd_resp_send_err(req, http_status, NULL),
        exit,
        __func__,
        "failed to send HTTP 500"
    );
  }

exit:
  close(fd);
  return ret;
}
