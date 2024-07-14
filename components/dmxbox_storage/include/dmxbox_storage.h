#pragma once
#include "channel_types.h"
#include "effect_step_storage.h"
#include "effect_storage.h"
#include "entry.h"
#include "universe_storage.h"
#include <esp_err.h>
#include <nvs.h>
#include <stdbool.h>
#include <stdint.h>

void dmxbox_storage_init();
void dmxbox_storage_factory_reset();

uint8_t dmxbox_get_first_run_completed();
uint8_t dmxbox_get_sta_mode_enabled();
const char *dmxbox_get_hostname();
const char *dmxbox_get_default_hostname();

void dmxbox_set_first_run_completed(uint8_t value);
void dmxbox_set_sta_mode_enabled(uint8_t value);
bool dmxbox_set_hostname(const char *value);
