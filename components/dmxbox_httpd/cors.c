#include "dmxbox_httpd.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "http_parser.h"
#include "sdkconfig.h"
#include <esp_check.h>

#define CONFIG_CORS_ORIGIN "http://localhost:8000"
#ifdef CONFIG_CORS_ORIGIN
static const char *dmxbox_httpd_cors_origin = CONFIG_CORS_ORIGIN;
#else
static const char *dmxbox_httpd_cors_origin = NULL;
#endif

static const char TAG[] = "dmxbox_httpd_cors";

esp_err_t dmxbox_httpd_cors_allow_origin(httpd_req_t *req) {
  if (dmxbox_httpd_cors_origin) {
    ESP_RETURN_ON_ERROR(
        httpd_resp_set_hdr(
            req,
            "Access-Control-Allow-Origin",
            dmxbox_httpd_cors_origin
        ),
        TAG,
        "failed to set allow origin header"
    );
  }
  return ESP_OK;
}

esp_err_t
dmxbox_httpd_cors_allow_methods(httpd_req_t *req, const char *methods) {
  if (dmxbox_httpd_cors_origin) {
    ESP_RETURN_ON_ERROR(
        httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", methods),
        TAG,
        "failed to set allow methods header"
    );
  }
  return ESP_OK;
}

esp_err_t dmxbox_httpd_cors_options_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "OPTIONS request for %s", req->uri);
  ESP_RETURN_ON_ERROR(
      dmxbox_httpd_cors_allow_origin(req),
      TAG,
      "failed to set allow origin header"
  );
  ESP_RETURN_ON_ERROR(
      dmxbox_httpd_cors_allow_methods(req, "GET,PUT,POST,DELETE"),
      TAG,
      "failed to set allow method headers"
  );
  ESP_RETURN_ON_ERROR(
      httpd_resp_send(req, "", 0),
      TAG,
      "httpd_resp_send failed"
  );
  return ESP_OK;
}

esp_err_t
dmxbox_httpd_cors_register_options(httpd_handle_t server, const char *path) {
  if (dmxbox_httpd_cors_origin) {
    const httpd_uri_t uri = {
        .uri = path,
        .method = HTTP_OPTIONS,
        .handler = dmxbox_httpd_cors_options_handler,
    };
    return httpd_register_uri_handler(server, &uri);
  } else {
    return ESP_OK;
  }
}
