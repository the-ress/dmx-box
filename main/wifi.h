#include "freertos/event_groups.h"
#include "esp_wifi.h"

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_STA_CONNECTED_BIT BIT0
#define WIFI_STA_DISCONNECTED_BIT BIT1
#define WIFI_STA_FAIL_BIT BIT2
#define WIFI_AP_STARTED_BIT BIT3
#define WIFI_AP_STOPPED_BIT BIT4
#define WIFI_SHOULD_ADVERTISE_ARTNET_AP BIT5
#define WIFI_SHOULD_ADVERTISE_ARTNET_STA BIT6

extern struct wifi_config
{
    struct
    {
        char ssid[32];
        char password[64];
        wifi_auth_mode_t auth_mode;
        uint8_t channel;
    } ap;

    struct
    {
        char ssid[32];
        char password[64];
        wifi_auth_mode_t auth_mode;
    } sta;
} current_wifi_config;

/* FreeRTOS event group to signal when we are connected*/
extern EventGroupHandle_t wifi_event_group;

void wifi_start();
void wifi_set_defaults(void);
void wifi_update_config(const struct wifi_config *new_config, bool sta_mode_enabled);

esp_netif_t *wifi_get_ap_interface(void);
esp_netif_t *wifi_get_sta_interface(void);
