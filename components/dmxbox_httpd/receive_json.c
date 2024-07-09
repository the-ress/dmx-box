#include "receive_json.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "scratch.h"
#include <esp_log.h>

static const char TAG[] = "dmxbox_httpd_request";

esp_err_t dmxbox_httpd_receive_json(httpd_req_t *req, cJSON **json) {
  if (!json) {
    return ESP_ERR_INVALID_ARG;
  }

  if (req->content_len >= sizeof(dmxbox_httpd_scratch)) {
    ESP_RETURN_ON_ERROR(
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Content too long"),
        TAG, "failed to send 400 Content too long");
    return ESP_OK;
  }

  int received =
      httpd_req_recv(req, dmxbox_httpd_scratch, sizeof(dmxbox_httpd_scratch));

  switch (received) {
  case HTTPD_SOCK_ERR_INVALID:
  case HTTPD_SOCK_ERR_FAIL:
    ESP_LOGE(TAG, "failed to read from the socket");
    return ESP_FAIL;

  case HTTPD_SOCK_ERR_TIMEOUT:
    ESP_RETURN_ON_ERROR(httpd_resp_send_err(req, HTTPD_408_REQ_TIMEOUT, NULL),
                        TAG, "failed to send 408 Request Timeout");
    return ESP_FAIL;

  default:
    ESP_LOGI(TAG, "received %d bytes", received);
    break;
  }

  *json = cJSON_ParseWithLength(dmxbox_httpd_scratch, received);
  if (!*json) {
    dmxbox_httpd_scratch[received] = 0;
    ESP_LOGD(TAG, "failed to parse JSON: %s", dmxbox_httpd_scratch);
    ESP_RETURN_ON_ERROR(
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request"), TAG,
        "failed to send 400 Bad Request");
    return ESP_OK;
  }

  return ESP_OK;
}
