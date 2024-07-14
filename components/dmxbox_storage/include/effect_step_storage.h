#pragma once
#include "channel_types.h"
#include "effect_storage.h"
#include "universe_storage.h"
#include <esp_err.h>
#include <stddef.h>
#include <stdint.h>

typedef struct dmxbox_channel_level {
  dmxbox_channel_t channel;
  uint8_t level;
} dmxbox_channel_level_t;

typedef struct dmxbox_effect_step {
  uint32_t time;
  uint32_t in;
  uint32_t dwell;
  uint32_t out;
  size_t channel_count;
  dmxbox_channel_level_t channels[1];
} dmxbox_effect_step_t;

dmxbox_effect_step_t *dmxbox_effect_step_alloc(size_t channel_count);

// result can be NULL
// caller must free() *result if they provided one and this returns ESP_OK
esp_err_t dmxbox_effect_step_get(
    uint16_t effect_id,
    uint16_t step_id,
    dmxbox_effect_step_t **result
);

esp_err_t dmxbox_effect_step_set(
    uint16_t effect_id,
    uint16_t step_id,
    const dmxbox_effect_step_t *value
);
