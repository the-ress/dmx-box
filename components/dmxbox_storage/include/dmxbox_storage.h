#pragma once
#include <esp_err.h>
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

typedef union dmxbox_storage_universe {
  unsigned address : 15;
  struct {
    unsigned net : 7;
    unsigned subnet : 4;
    unsigned universe : 4;
  };
} dmxbox_storage_universe_t;

typedef struct dmxbox_storage_channel {
  dmxbox_storage_universe_t universe;
  unsigned index : 9;
} dmxbox_storage_channel_t;

typedef struct dmxbox_storage_channel_level {
  dmxbox_storage_channel_t channel;
  uint8_t level;
} dmxbox_storage_channel_level_t;

typedef struct dmxbox_storage_effect_step {
  uint32_t time;
  uint32_t in;
  uint32_t dwell;
  uint32_t out;
  dmxbox_storage_channel_level_t channels[1];
} dmxbox_storage_effect_step_t;

size_t dmxbox_storage_effect_step_size(size_t channel_count);

size_t dmxbox_storage_effect_step_channel_count(size_t blob_size);

dmxbox_storage_effect_step_t *
dmxbox_storage_effect_step_alloc(size_t channel_count);

// caller must free() the result if ESP_OK
esp_err_t dmxbox_storage_effect_step_get(
    uint16_t effect_id,
    uint16_t step_id,
    size_t *channel_count,
    dmxbox_storage_effect_step_t **result
);

esp_err_t dmxbox_storage_effect_step_set(
    uint16_t effect_id,
    uint16_t step_id,
    size_t channel_count,
    const dmxbox_storage_effect_step_t *result
);
