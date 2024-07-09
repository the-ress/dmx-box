#pragma once
#include <esp_err.h>
#include <esp_http_server.h>

esp_err_t dmxbox_api_cors_allow_origin(httpd_req_t *req);
esp_err_t dmxbox_api_cors_allow_methods(httpd_req_t *req, const char *methods);
