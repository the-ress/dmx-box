#include "dmxbox_httpd.h"
#include "esp_http_server.h"
#include <esp_check.h>

static const char TAG[] = "dmxbox_httpd_responses";

static esp_err_t
dmxbox_httpd_send_response(httpd_req_t *req, const char *status) {
  ESP_RETURN_ON_ERROR(
      httpd_resp_set_status(req, status),
      TAG,
      "failed to set status"
  );
  return httpd_resp_send(req, NULL, 0);
}

esp_err_t dmxbox_httpd_send_201_created(httpd_req_t *req) {
  return dmxbox_httpd_send_response(req, "201 Created");
}

esp_err_t dmxbox_httpd_send_204_no_content(httpd_req_t *req) {
  return dmxbox_httpd_send_response(req, "204 No Content");
}
