#include "sdkconfig.h"
#include "esp_dmx.h"

#define RESET_BUTTON_GPIO 2
#define WIFI_MODE_GPIO 4

#define DMX_OUT_GPIO 13
#define DMX_IN_GPIO 17

#define DMX_OUT_NUM DMX_NUM_1 // Use UART 1
#define DMX_IN_NUM DMX_NUM_2 // Use UART 2

#define DMX_CHANNEL_COUNT (DMX_PACKET_SIZE_MAX - 1)
