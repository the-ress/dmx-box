#pragma once
#include <cJSON.h>
#include <esp_wifi.h>
#include <stdbool.h>

const char *dmxbox_auth_mode_to_str(wifi_auth_mode_t auth_mode);
bool dmxbox_auth_mode_from_str(const char *str, wifi_auth_mode_t *result);
cJSON *dmxbox_mac_to_json(const uint8_t mac[6]);
