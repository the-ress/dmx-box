#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "freertos/event_groups.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "sdkconfig.h"
#include "const.h"
#include "led.h"
#include "wifi.h"
#include "storage.h"

static const char *TAG = "wifi-setup";

#define CONFIG_WIFI_AP_SSID "DmxBox_"
#define CONFIG_WIFI_AP_PASSWORD "cue-gobo-fresnel"
#define CONFIG_WIFI_AP_AUTH_MODE WIFI_AUTH_WPA2_PSK
#define CONFIG_WIFI_AP_CIPHER_TYPE WIFI_CIPHER_TYPE_CCMP
#define CONFIG_WIFI_AP_CHANNEL 6
#define CONFIG_WIFI_AP_MAX_STA_CONN 5

#define CONFIG_WIFI_STA_SSID ""
#define CONFIG_WIFI_STA_PASSWORD ""
#define CONFIG_WIFI_STA_AUTH_MODE WIFI_AUTH_WPA2_PSK
#define CONFIG_WIFI_STA_MAXIMUM_RETRY 5

static int retry_num = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START)
    {
        ESP_LOGI(TAG, "AP started");
        xEventGroupClearBits(wifi_event_group, WIFI_AP_STOPPED_BIT);
        xEventGroupSetBits(wifi_event_group, WIFI_AP_STARTED_BIT);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STOP)
    {
        ESP_LOGI(TAG, "AP stopped");
        xEventGroupClearBits(wifi_event_group, WIFI_AP_STARTED_BIT);
        xEventGroupSetBits(wifi_event_group, WIFI_AP_STOPPED_BIT);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);

        led_set_state(AP_MODE_LED_GPIO, 1);

        xEventGroupSetBits(wifi_event_group, WIFI_SHOULD_ADVERTISE_ARTNET_AP);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);

        wifi_sta_list_t sta_list;
        esp_wifi_ap_get_sta_list(&sta_list);

        ESP_LOGI(TAG, "%d stations remain connected ", sta_list.num);

        if (sta_list.num == 0)
        {
            led_set_state(AP_MODE_LED_GPIO, 0);
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "disconnected from AP as STA");
        led_set_state(CLIENT_MODE_LED_GPIO, 0);
        xEventGroupClearBits(wifi_event_group, WIFI_STA_CONNECTED_BIT);
        xEventGroupSetBits(wifi_event_group, WIFI_STA_DISCONNECTED_BIT);

        if (get_sta_mode_enabled())
        {
            if (retry_num < CONFIG_WIFI_STA_MAXIMUM_RETRY)
            {
                esp_wifi_connect();
                retry_num++;
                ESP_LOGI(TAG, "retry to connect to AP as STA");
            }
            else
            {
                xEventGroupSetBits(wifi_event_group, WIFI_STA_FAIL_BIT);
            }
            ESP_LOGI(TAG, "connect to AP as STA fail");
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        led_set_state(CLIENT_MODE_LED_GPIO, 1);

        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        retry_num = 0;
        xEventGroupClearBits(wifi_event_group, WIFI_STA_DISCONNECTED_BIT);
        xEventGroupSetBits(wifi_event_group, WIFI_STA_CONNECTED_BIT | WIFI_SHOULD_ADVERTISE_ARTNET_STA);
    }
}

void wifi_init(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();

    // disable wifi power save to improve performance
    esp_wifi_set_ps(WIFI_PS_NONE);
}

void refresh_cached_wifi_config(const wifi_config_t *wifi_ap_config, const wifi_config_t *wifi_sta_config)
{
    memcpy(current_wifi_config.ap.ssid, wifi_ap_config->ap.ssid, sizeof(current_wifi_config.ap.ssid));
    memcpy(current_wifi_config.ap.password, wifi_ap_config->ap.password, sizeof(current_wifi_config.ap.password));
    current_wifi_config.ap.auth_mode = wifi_ap_config->ap.authmode;
    current_wifi_config.ap.channel = wifi_ap_config->ap.channel;

    memcpy(current_wifi_config.sta.ssid, wifi_sta_config->sta.ssid, sizeof(current_wifi_config.sta.ssid));
    memcpy(current_wifi_config.sta.password, wifi_sta_config->sta.password, sizeof(current_wifi_config.sta.password));
    current_wifi_config.sta.auth_mode = wifi_sta_config->sta.threshold.authmode;
}

void wifi_start()
{
    wifi_init();

    uint8_t sta_mode_enabled = get_sta_mode_enabled();

    ESP_ERROR_CHECK(esp_wifi_set_mode(sta_mode_enabled ? WIFI_MODE_APSTA : WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_start finished");

    wifi_config_t wifi_ap_config;
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_AP, &wifi_ap_config));
    wifi_config_t wifi_sta_config;
    ESP_ERROR_CHECK(esp_wifi_get_config(WIFI_IF_STA, &wifi_sta_config));

    refresh_cached_wifi_config(&wifi_ap_config, &wifi_sta_config);

    ESP_LOGI(TAG, "AP SSID: %s password: %s auth_mode: %d channel: %d", wifi_ap_config.ap.ssid, wifi_ap_config.ap.password, wifi_ap_config.ap.authmode, wifi_ap_config.ap.channel);
    if (sta_mode_enabled)
    {
        ESP_LOGI(TAG, "STA SSID: %s password: %s auth_mode: %d", wifi_sta_config.sta.ssid, wifi_sta_config.sta.password, wifi_sta_config.sta.threshold.authmode);
    }
    else
    {
        ESP_LOGI(TAG, "STA mode is disabled");
    }

    // /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
    //  * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    // EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
    //                                        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
    //                                        pdFALSE,
    //                                        pdFALSE,
    //                                        portMAX_DELAY);

    // /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
    //  * happened. */
    // if (bits & WIFI_CONNECTED_BIT)
    // {
    //     ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
    //              CONFIG_WIFI_STA_SSID, CONFIG_WIFI_STA_PASSWORD);
    // }
    // else if (bits & WIFI_FAIL_BIT)
    // {
    //     ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
    //              CONFIG_WIFI_STA_SSID, CONFIG_WIFI_STA_PASSWORD);
    // }
    // else
    // {
    //     ESP_LOGE(TAG, "UNEXPECTED EVENT");
    // }

    // vTaskDelete(NULL);
}

void wifi_set_defaults(void)
{
    ESP_LOGI(TAG, "Setting wifi configuration to default values");

    wifi_mode_t original_wifi_mode;
    ESP_ERROR_CHECK(esp_wifi_get_mode(&original_wifi_mode));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    uint8_t mac[6];
    ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP));

    wifi_config_t wifi_ap_config = {
        .ap = {
            .channel = CONFIG_WIFI_AP_CHANNEL,
            .password = CONFIG_WIFI_AP_PASSWORD,
            .max_connection = CONFIG_WIFI_AP_MAX_STA_CONN,
            .authmode = CONFIG_WIFI_AP_AUTH_MODE,
            .pairwise_cipher = CONFIG_WIFI_AP_CIPHER_TYPE,
            // .pmf_cfg = {
            //     .required = false,
            // },
        },
    };

    sprintf((char*)wifi_ap_config.ap.ssid, "%s%x%x%x", CONFIG_WIFI_AP_SSID, mac[3], mac[4], mac[5]);
    wifi_ap_config.ap.ssid_len = strlen((const char*)wifi_ap_config.ap.ssid);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    wifi_config_t wifi_sta_config = {
        .sta = {
            .ssid = CONFIG_WIFI_STA_SSID,
            .password = CONFIG_WIFI_STA_PASSWORD,
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
            .threshold.authmode = CONFIG_WIFI_STA_AUTH_MODE,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));

    ESP_ERROR_CHECK(esp_wifi_set_mode(original_wifi_mode));

    set_sta_mode_enabled(0);

    refresh_cached_wifi_config(&wifi_ap_config, &wifi_sta_config);
}

void wifi_update_config(const struct wifi_config *new_config, bool sta_mode_enabled)
{
    ESP_LOGI(TAG, "Updating wifi config");

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid_len = strlen(new_config->ap.ssid),
            .channel = new_config->ap.channel,
            .max_connection = CONFIG_WIFI_AP_MAX_STA_CONN,
            .authmode = new_config->ap.auth_mode,
            .pairwise_cipher = WIFI_CIPHER_TYPE_CCMP,
            // .pmf_cfg = {
            //     .required = false,
            // },
        },
    };

    const char *ap_password = new_config->ap.auth_mode != WIFI_AUTH_OPEN ? new_config->ap.password : "";
    strlcpy((char *)wifi_ap_config.ap.ssid, new_config->ap.ssid, sizeof(wifi_ap_config.ap.ssid));
    strlcpy((char *)wifi_ap_config.ap.password, ap_password, sizeof(wifi_ap_config.ap.password));

    if (strlen(CONFIG_WIFI_AP_PASSWORD) == 0)
    {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config)); // TODO handle errors

    wifi_config_t wifi_sta_config = {
        .sta = {
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
            .threshold.authmode = new_config->sta.auth_mode,
        },
    };

    const char *sta_password = new_config->sta.auth_mode != WIFI_AUTH_OPEN ? new_config->sta.password : "";
    strlcpy((char *)wifi_sta_config.sta.ssid, new_config->sta.ssid, sizeof(wifi_sta_config.sta.ssid));
    strlcpy((char *)wifi_sta_config.sta.password, sta_password, sizeof(wifi_sta_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config)); // TODO handle errors

    set_sta_mode_enabled(sta_mode_enabled);

    ESP_ERROR_CHECK(esp_wifi_set_mode(sta_mode_enabled ? WIFI_MODE_APSTA : WIFI_MODE_AP));

    refresh_cached_wifi_config(&wifi_ap_config, &wifi_sta_config);

    if (sta_mode_enabled)
    {
        ESP_ERROR_CHECK(esp_wifi_disconnect());
        ESP_ERROR_CHECK(esp_wifi_connect());
    }
}
