#pragma once
#include "dmxbox_const.h"
#include "esp_dmx.h"

extern portMUX_TYPE dmxbox_dmx_in_spinlock;
extern uint8_t dmxbox_dmx_in_data[DMX_PACKET_SIZE_MAX];
extern bool dmxbox_dmx_in_connected;

void dmxbox_dmx_receive_task(void *parameter);
void dmxbox_dmx_receive_get_data(uint8_t data[DMX_CHANNEL_COUNT]);
