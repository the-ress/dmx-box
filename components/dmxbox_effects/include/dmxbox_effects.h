#pragma once
#include "dmxbox_const.h"

extern portMUX_TYPE dmxbox_effects_spinlock;
extern uint8_t dmxbox_effects_data[DMX_CHANNEL_COUNT];

void dmxbox_effects_init();
void dmxbox_effects_task(void *parameter);
