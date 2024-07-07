#pragma once
#include <esp_err.h>
#include <esp_http_server.h>

const char *dmxbox_httpd_type_from_path(const char *path);
const char *dmxbox_httpd_validate_uri(const char *uri);
esp_err_t dmxbox_httpd_static_handler(httpd_req_t *req);
