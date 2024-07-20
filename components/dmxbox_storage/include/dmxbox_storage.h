#pragma once
#include <esp_err.h>
#include <nvs.h>
#include <stdbool.h>
#include <stdint.h>

#include "channel_types.h"
#include "dmxbox_const.h"
#include "effect_step_storage.h"
#include "effect_storage.h"
#include "entry.h"
#include "universe_storage.h"

void dmxbox_storage_init();
void dmxbox_storage_set_defaults();
void dmxbox_storage_factory_reset();

uint8_t dmxbox_get_first_run_completed();
uint8_t dmxbox_get_sta_mode_enabled();
const char *dmxbox_get_hostname();
const char *dmxbox_get_default_hostname();

uint16_t dmxbox_get_native_universe();
uint16_t dmxbox_get_effect_control_universe();
bool dmxbox_get_artnet_snapshot(
    uint16_t universe,
    uint8_t data[DMX_CHANNEL_COUNT]
);

void dmxbox_set_first_run_completed(uint8_t value);
void dmxbox_set_sta_mode_enabled(uint8_t value);
bool dmxbox_set_hostname(const char *value);

void dmxbox_set_native_universe(uint16_t value);
void dmxbox_set_effect_control_universe(uint16_t value);
void dmxbox_set_artnet_snapshot(
    uint16_t universe,
    const uint8_t data[DMX_CHANNEL_COUNT]
);
