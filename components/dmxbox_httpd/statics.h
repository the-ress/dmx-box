#pragma once
#include <esp_err.h>
#include <esp_http_server.h>

esp_err_t dmxbox_httpd_statics_register(httpd_handle_t server);
