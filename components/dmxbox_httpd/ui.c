#include "ui.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "private/scratch.h"
#include <assert.h>
#include <esp_check.h>
#include <esp_vfs.h>
#include <string.h>
#include <sys/_default_fcntl.h>
#include <sys/fcntl.h>

#ifndef CONFIG_DMXBOX_HTTPD_STATICS_ROOT
// must end with /
#define CONFIG_DMXBOX_HTTPD_STATICS_ROOT "/www/"
#endif

#ifndef CONFIG_DMXBOX_HTTPD_STATICS_INDEX
#define CONFIG_DMXBOX_HTTPD_STATICS_INDEX                                      \
  CONFIG_DMXBOX_HTTPD_STATICS_ROOT "index.html"
#endif

static const char TAG[] = "dmxbox_httpd_statics";

static const char *dmxbox_httpd_type_from_filename(const char *filename) {
  const char *ext = strrchr(filename, '.');
  if (!ext) {
    return HTTPD_TYPE_OCTET;
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
  return HTTPD_TYPE_OCTET;
}

// strips leading /, ensures path contains no further / and no ..
static const char *dmxbox_httpd_get_static_filename(const char *uri) {
  if (*uri == '/') {
    uri++;
  }
  if (*uri == '\0') {
    return NULL;
  }
  const char *ch = uri;
  while (*ch) {
    if (*ch == '/') {
      return NULL;
    } else if (ch[0] == '.' && ch[1] == '.') {
      return NULL;
    }
    ch++;
  }
  return uri;
}

static int dmxbox_httpd_open_static_file(const char *filename) {
  char path[260] = CONFIG_DMXBOX_HTTPD_STATICS_ROOT;
  if (strlcat(path, filename, sizeof(path)) >= sizeof(path)) {
    ESP_LOGE(TAG, "filename too long");
    return -1;
  }
  ESP_LOGI(TAG, "trying %s", path);
  return open(path, O_RDONLY, 0);
}

static esp_err_t dmxbox_ui_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "Handling GET %s", req->uri);

  esp_err_t ret = ESP_OK;
  const char *filename = dmxbox_httpd_get_static_filename(req->uri);
  int fd = -1;

  if (filename) {
    fd = dmxbox_httpd_open_static_file(filename);
  }
  if (fd == -1) {
    ESP_LOGI(
        TAG,
        "could not find a file for that url, "
        "using " CONFIG_DMXBOX_HTTPD_STATICS_INDEX
    );
    filename = CONFIG_DMXBOX_HTTPD_STATICS_INDEX;
    fd = open(filename, O_RDONLY, 0);
  }
  if (fd == -1) {
    return httpd_resp_send_404(req);
  }

  const char *type = dmxbox_httpd_type_from_filename(filename);
  ESP_LOGI(TAG, "MIME type: %s", type);
  ESP_GOTO_ON_ERROR(
      httpd_resp_set_type(req, type),
      exit,
      TAG,
      "httpd_resp_set_type failed"
  );

  do {
    size_t read_bytes =
        read(fd, dmxbox_httpd_scratch, sizeof(dmxbox_httpd_scratch));
    switch (read_bytes) {
    case -1:
      ESP_LOGE(TAG, "Failed to read file %s", filename);
      ret = ESP_FAIL;
      goto send_response;

    case 0:
      ESP_LOGI(TAG, "File sending complete: %s", filename);
      goto send_response;

    default:
      ESP_LOGD(TAG, "Sending %d-byte chunk of %s", read_bytes, filename);
      ESP_GOTO_ON_ERROR(
          httpd_resp_send_chunk(req, dmxbox_httpd_scratch, read_bytes),
          exit,
          TAG,
          "http_resp_send_chunk failed"
      );
    }
  } while (ret == ESP_OK);

send_response:
  if (ret == ESP_OK) {
    ESP_GOTO_ON_ERROR(
        httpd_resp_send_chunk(req, NULL, 0),
        exit,
        TAG,
        "failed to send the final chunk"
    );
  } else {
    ESP_GOTO_ON_ERROR(
        httpd_resp_send_500(req),
        exit,
        TAG,
        "failed to send HTTP 500"
    );
  }

exit:
  close(fd);
  return ret;
}

esp_err_t dmxbox_ui_register(httpd_handle_t server) {
  static const httpd_uri_t uri = {
      .uri = "/*",
      .method = HTTP_GET,
      .handler = dmxbox_ui_handler,
  };
  return httpd_register_uri_handler(server, &uri);
}
