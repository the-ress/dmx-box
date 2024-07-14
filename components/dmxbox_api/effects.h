#pragma once
#include <esp_err.h>
#include <esp_http_server.h>

esp_err_t dmxbox_api_effect_endpoint(httpd_req_t *req, uint16_t effect_id) ;

