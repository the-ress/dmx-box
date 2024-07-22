#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "dmxbox_const.h"

extern portMUX_TYPE dmxbox_artnet_spinlock;
const uint8_t *dmxbox_artnet_get_native_universe_data();
bool dmxbox_artnet_get_universe_data(
    uint16_t address,
    uint8_t data[DMX_CHANNEL_COUNT]
);

void dmxbox_artnet_init();

void dmxbox_artnet_receive_task(void *parameter);
void dmxbox_set_artnet_active(bool state);
