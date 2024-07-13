#pragma once
#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>

void dmxbox_storage_init(void);
void dmxbox_storage_factory_reset(void);

uint8_t dmxbox_get_first_run_completed(void);
uint8_t dmxbox_get_sta_mode_enabled(void);
const char *dmxbox_get_hostname(void);
const char *dmxbox_get_default_hostname(void);

void dmxbox_set_first_run_completed(uint8_t value);
void dmxbox_set_sta_mode_enabled(uint8_t value);
bool dmxbox_set_hostname(const char *value);

typedef struct dmxbox_storage_channel_level {
  unsigned universe : 15;
  unsigned channel : 9;
  unsigned level : 8;
} dmxbox_storage_channel_level_t;

typedef struct dmxbox_storage_effect_step {
  uint32_t time;
  uint32_t in;
  uint32_t dwell;
  uint32_t out;
  dmxbox_storage_channel_level_t channels[1];
} dmxbox_storage_effect_step_t;

// caller must free() the result if ESP_OK
esp_err_t dmxbox_storage_get_effect_step(
    uint16_t effect_id,
    uint16_t step_id,
    size_t *channel_count,
    dmxbox_storage_effect_step_t **result
);

esp_err_t dmxbox_storage_put_effect_step(
    uint16_t effect_id,
    uint16_t step_id,
    size_t channel_count,
    const dmxbox_storage_effect_step_t *result
);
