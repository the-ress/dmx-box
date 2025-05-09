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

typedef struct dmxbox_wifi_sta {
  bool enabled;
  char ssid[33]; // including zero terminator
} dmxbox_wifi_sta_t;

extern dmxbox_wifi_config_t dmxbox_wifi_config;
extern EventGroupHandle_t dmxbox_wifi_event_group;

void dmxbox_wifi_start();

esp_err_t dmxbox_wifi_get_sta(dmxbox_wifi_sta_t *sta);
esp_err_t dmxbox_wifi_disable_sta();
esp_err_t dmxbox_wifi_enable_sta(
    const char *ssid,
    wifi_auth_mode_t auth_mode,
    const char *password
);
void wifi_set_defaults();
void wifi_update_config(
    const dmxbox_wifi_config_t *new_config,
    bool sta_mode_enabled
);

esp_netif_t *wifi_get_ap_interface();
esp_netif_t *wifi_get_sta_interface();
