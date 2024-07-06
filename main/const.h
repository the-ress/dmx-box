#include "sdkconfig.h"
#include "esp_dmx.h"

// #define PWR_GPIO CONFIG_PWR_GPIO
#define POWER_LED_GPIO 18
#define DMX_OUT_LED_GPIO 14
#define DMX_IN_LED_GPIO 19
#define AP_MODE_LED_GPIO 22
#define CLIENT_MODE_LED_GPIO 27
#define ARTNET_IN_LED_GPIO 21

#define RESET_BUTTON_GPIO 2
#define WIFI_MODE_GPIO 4

#define DMX_OUT_GPIO 13
#define DMX_IN_GPIO 17

#define DMX_OUT_NUM DMX_NUM_1 // Use UART 1
#define DMX_IN_NUM DMX_NUM_2 // Use UART 2

#define DMX_CHANNEL_COUNT (DMX_PACKET_SIZE_MAX - 1)
