#pragma once
#include "universe_storage.h"
#include <esp_err.h>
#include <stddef.h>
#include <stdint.h>

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
  size_t channel_count;
  dmxbox_storage_channel_level_t channels[1];
} dmxbox_storage_effect_step_t;

dmxbox_storage_effect_step_t *
dmxbox_storage_effect_step_alloc(size_t channel_count);

// result can be NULL
// caller must free() *result if they provided one and this returns ESP_OK
esp_err_t dmxbox_storage_effect_step_get(
    uint16_t effect_id,
    uint16_t step_id,
    dmxbox_storage_effect_step_t **result
);

esp_err_t dmxbox_storage_effect_step_set(
    uint16_t effect_id,
    uint16_t step_id,
    const dmxbox_storage_effect_step_t *result
);
