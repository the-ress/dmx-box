#pragma once
#include "dmxbox_storage.h"
#include <cJSON.h>
#include <esp_wifi.h>
#include <stdbool.h>

const char *dmxbox_auth_mode_to_str(wifi_auth_mode_t auth_mode);
bool dmxbox_auth_mode_from_str(const char *str, wifi_auth_mode_t *result);
cJSON *dmxbox_mac_to_json(const uint8_t mac[6]);

cJSON *dmxbox_api_channel_to_json(const dmxbox_channel_t *c);
cJSON *dmxbox_api_optional_channel_to_json(const dmxbox_channel_t *c);
bool dmxbox_api_channel_from_json(const cJSON *json, dmxbox_channel_t *c);
bool dmxbox_api_optional_channel_from_json(
    const cJSON *json,
    dmxbox_channel_t *c
);
