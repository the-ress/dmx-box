#pragma once
#include <esp_err.h>
#include <esp_http_server.h>
#include <stdint.h>

esp_err_t dmxbox_api_effect_step_get(
    httpd_req_t *req,
    uint16_t effect_id,
    uint16_t step_id
);
