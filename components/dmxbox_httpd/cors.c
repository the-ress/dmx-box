#include "dmxbox_httpd.h"
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
esp_err_t dmxbox_httpd_cors_allow_methods(httpd_req_t *req, const char *methods) {
  if (dmxbox_httpd_cors_origin) {
    ESP_RETURN_ON_ERROR(
        httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", methods),
        TAG,
        "failed to set allow methods header"
    );
  }
  return ESP_OK;
}
