#pragma once
#include <stdbool.h>
#include <stdint.h>

void dmxbox_storage_init(void);
void dmxbox_storage_factory_reset(void);

uint8_t dmxbox_get_first_run_completed(void);
uint8_t dmxbox_get_sta_mode_enabled(void);
const char *dmxbox_get_hostname(void);

void dmxbox_set_first_run_completed(uint8_t value);
void dmxbox_set_sta_mode_enabled(uint8_t value);
bool dmxbox_set_hostname(const char *value);
