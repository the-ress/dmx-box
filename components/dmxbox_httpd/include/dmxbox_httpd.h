#pragma once
#include "sdkconfig.h"
#include <cJSON.h>
#include <esp_err.h>
#include <esp_http_server.h>

#ifndef CONFIG_DMXBOX_WEBSEVER_MAX_SOCKETS
#define CONFIG_DMXBOX_WEBSEVER_MAX_SOCKETS 10
#endif

esp_err_t dmxbox_httpd_start();

esp_err_t dmxbox_httpd_cors_allow_origin(httpd_req_t *req);
esp_err_t
dmxbox_httpd_cors_allow_methods(httpd_req_t *req, const char *methods);
esp_err_t
dmxbox_httpd_cors_register_options(httpd_handle_t server, const char *path);

esp_err_t dmxbox_httpd_receive_json(httpd_req_t *req, cJSON **json);

esp_err_t dmxbox_httpd_send_201_created(httpd_req_t *req);
esp_err_t dmxbox_httpd_send_204_no_content(httpd_req_t *req);
esp_err_t dmxbox_httpd_send_json(httpd_req_t *req, cJSON *json);
esp_err_t dmxbox_httpd_send_jsonstr(httpd_req_t *req, const char *json);

esp_err_t dmxbox_httpd_statics_register(httpd_handle_t server);

