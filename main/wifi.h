#include <esp_err.h>
#include <esp_wifi.h>
#include <freertos/event_groups.h>
#include <stdbool.h>
#include <stdint.h>

enum dmxbox_wifi_events {
  dmxbox_wifi_sta_connected = 0x0001,
  dmxbox_wifi_sta_disconnected = 0x0002,
  dmxbox_wifi_sta_fail = 0x0004,
  dmxbox_wifi_ap_started = 0x0010,
  dmxbox_wifi_ap_stopped = 0x0020,
  dmxbox_wifi_ap_sta_connected = 0x0040,
};

typedef struct dmxbox_wifi_config {
  struct {
    char ssid[32];
    char password[64];
    wifi_auth_mode_t auth_mode;
    uint8_t channel;
  } ap;

  struct {
    char ssid[32];
    char password[64];
    wifi_auth_mode_t auth_mode;
  } sta;
} dmxbox_wifi_config_t;

extern dmxbox_wifi_config_t dmxbox_wifi_config;
extern EventGroupHandle_t dmxbox_wifi_event_group;

void dmxbox_wifi_start();
void wifi_set_defaults(void);
void wifi_update_config(
    const dmxbox_wifi_config_t *new_config,
    bool sta_mode_enabled
);

esp_netif_t *wifi_get_ap_interface(void);
esp_netif_t *wifi_get_sta_interface(void);
