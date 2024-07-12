#pragma once
#include "esp_dmx.h"

extern portMUX_TYPE dmxbox_dmx_out_spinlock;
extern uint8_t dmxbox_dmx_out_data[DMX_PACKET_SIZE_MAX];

void dmxbox_dmx_send_task(void *parameter);
void dmxbox_set_dmx_out_active(bool state);
