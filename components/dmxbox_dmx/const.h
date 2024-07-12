#pragma once
#include "esp_dmx.h"

typedef enum dmxbox_dmx_gpio {
  dmxbox_dmx_gpio_out = 13,
  dmxbox_dmx_gpio_in = 17,
} dmxbox_dmx_gpio_t;

#define DMX_OUT_NUM DMX_NUM_1 // Use UART 1
#define DMX_IN_NUM DMX_NUM_2  // Use UART 2
