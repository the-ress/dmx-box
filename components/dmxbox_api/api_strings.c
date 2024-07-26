#include "api_strings.h"
#include "cJSON.h"
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
    [WIFI_AUTH_WPA2_WPA3_PSK] = "WPA2_WPA3_PSK",
};

const char *dmxbox_auth_mode_to_str(wifi_auth_mode_t auth_mode) {
  const char *result = "";
  if (auth_mode >= 0 && auth_mode < WIFI_AUTH_MAX) {
    result = strings[auth_mode];
  }
  if (!result[0]) {
    ESP_LOGE(TAG, "Unsupported auth mode: %d", auth_mode);
    return NULL;
  }
  return result;
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

cJSON *dmxbox_api_channel_to_json(const dmxbox_channel_t *c) {
  char buffer[sizeof("127-16-16/512")];
  size_t size = c->universe.address
                    ? snprintf(
                          buffer,
                          sizeof(buffer),
                          "%u-%u-%u/%u",
                          c->universe.net,
                          c->universe.subnet,
                          c->universe.universe,
                          c->index
                      )
                    : snprintf(buffer, sizeof(buffer), "%u", c->index);
  return size < sizeof(buffer) ? cJSON_CreateString(buffer) : NULL;
}

cJSON *dmxbox_api_optional_channel_to_json(const dmxbox_channel_t *c) {
  if (c->index == 0 && c->universe.address == 0) {
    return cJSON_CreateNull();
  }

  return dmxbox_api_channel_to_json(c);
}

bool dmxbox_api_channel_from_json(const cJSON *json, dmxbox_channel_t *c) {
  if (!cJSON_IsString(json)) {
    ESP_LOGE(TAG, "channel is not a string");
    return false;
  }

  // TODO non-zero universes
  int value = atoi(json->valuestring);
  if (value < 1 || value > 512) {
    ESP_LOGE(TAG, "channel out of range: %d", value);
    return false;
  }

  c->index = value;
  return true;
}

bool dmxbox_api_optional_channel_from_json(
    const cJSON *json,
    dmxbox_channel_t *c
) {
  if (cJSON_IsNull(json)) {
    c->index = 0;
    return true;
  }

  return dmxbox_api_channel_from_json(json, c);
}
