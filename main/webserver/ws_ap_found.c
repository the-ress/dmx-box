#include "ws_ap_found.h"
#include <cJSON.h>
#include <esp_mac.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static cJSON *dmxbox_mac_to_json(const uint8_t *mac) {
  char buffer[18];
  if (snprintf(buffer, sizeof(buffer), MACSTR, MAC2STR(mac)) < sizeof(buffer)) {
    return cJSON_CreateString(buffer);
  }
  return NULL;
}

char *dmxbox_ws_ap_found_create(const wifi_ap_record_t *record) {
  char *text = NULL;
  cJSON *json = cJSON_CreateObject();
  if (!json) {
    goto fail;
  }
  if (!cJSON_AddStringToObject(json, "type", "settings/apFound")) {
    goto fail;
  }
  if (!cJSON_AddStringToObject(json, "ssid", (char*)record->ssid)) {
    goto fail;
  }
  if (!cJSON_AddNumberToObject(json, "rssi", record->rssi)) {
    goto fail;
  }
  cJSON *bssid = dmxbox_mac_to_json(record->bssid);
  if (!bssid) {
    goto fail;
  }
  if (!cJSON_AddItemToObject(json, "bssid", bssid)) {
    cJSON_Delete(bssid);
    goto fail;
  }
  text = cJSON_PrintUnformatted(json);
fail:
  if (json) {
    cJSON_Delete(json);
  }
  return text;
}
