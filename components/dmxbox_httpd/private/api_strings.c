#include "api_strings.h"
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_wifi_types.h>
#include <string.h>

static const char TAG[] = "dmxbox_api_strings";

static const char *strings[WIFI_AUTH_MAX] = {
    [WIFI_AUTH_OPEN] = "open",
    [WIFI_AUTH_WEP] = "WEP",
    [WIFI_AUTH_WPA_PSK] = "WPA_PSK",
    [WIFI_AUTH_WPA2_PSK] = "WPA2_PSK",
    [WIFI_AUTH_WPA_WPA2_PSK] = "WPA_WPA2_PSK",
    [WIFI_AUTH_WPA3_PSK] = "WPA3_PSK",
    [WIFI_AUTH_WPA2_WPA3_PSK] = "WPA2_WPA3_PSK"};

const char *dmxbox_auth_mode_to_str(wifi_auth_mode_t auth_mode) {
  if (auth_mode >= 0 && auth_mode < WIFI_AUTH_MAX) {
    return strings[auth_mode];
  }
  ESP_LOGE(TAG, "Unknown auth mode: %d", auth_mode);
  return "";
}

bool dmxbox_auth_mode_from_str(const char *str, wifi_auth_mode_t *result) {
  if (!result || !str || !*str) {
    return false;
  }
  for (size_t auth_mode = 0; auth_mode < WIFI_AUTH_MAX; auth_mode++) {
    if (strings[auth_mode] && !strcmp(strings[auth_mode], str)) {
      *result = auth_mode;
      return true;
    }
  }
  ESP_LOGE(TAG, "Unknown auth mode string: '%s'", str);
  return false;
}

cJSON *dmxbox_mac_to_json(const uint8_t mac[6]) {
  char buffer[18];
  if (snprintf(buffer, sizeof(buffer), MACSTR, MAC2STR(mac)) < sizeof(buffer)) {
    return cJSON_CreateString(buffer);
  }
  return NULL;
}
