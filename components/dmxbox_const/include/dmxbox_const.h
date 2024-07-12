#pragma once
#include "esp_dmx.h"

#define DMX_CHANNEL_COUNT (DMX_PACKET_SIZE_MAX - 1)

typedef enum dmxbox_button {
  dmxbox_button_reset = 2,
} dmxbox_button_t;
