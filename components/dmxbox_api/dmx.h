#pragma once
#include <esp_http_server.h>
#include <esp_err.h>

esp_err_t dmxbox_api_dmx_register(httpd_handle_t server);
