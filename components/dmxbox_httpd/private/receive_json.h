#pragma once
#include <cJSON.h>
#include <esp_err.h>
#include <esp_http_server.h>

esp_err_t dmxbox_httpd_receive_json(httpd_req_t *req, cJSON **json);

// will free the json
esp_err_t dmxbox_httpd_send_json(httpd_req_t *req, cJSON *json);
