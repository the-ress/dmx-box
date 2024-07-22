#pragma once
#include <stdbool.h>

#define CONFIG_WIFI_AP_SSID "DmxBox_"
#define CONFIG_WIFI_AP_PASSWORD "cue-gobo-fresnel"
#define CONFIG_WIFI_AP_AUTH_MODE WIFI_AUTH_WPA2_PSK
#define CONFIG_WIFI_AP_CIPHER_TYPE WIFI_CIPHER_TYPE_CCMP
#define CONFIG_WIFI_AP_CHANNEL 6
#define CONFIG_WIFI_AP_MAX_STA_CONN 5

#define CONFIG_WIFI_STA_SSID ""
#define CONFIG_WIFI_STA_PASSWORD ""
#define CONFIG_WIFI_STA_AUTH_MODE WIFI_AUTH_WPA2_PSK
#define CONFIG_WIFI_STA_MAXIMUM_RETRY 3
#define CONFIG_WIFI_STA_RETRY_INTERVAL 30000

extern bool is_in_disconnect;

void dmxbox_wifi_register_event_handlers();

void dmxbox_wifi_scan_on_done();
