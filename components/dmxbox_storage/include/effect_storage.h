#pragma once
#include "channel_types.h"
#include <esp_err.h>
#include <stddef.h>

typedef struct dmxbox_effect {
  char name[33];
  dmxbox_channel_t level_channel;
  dmxbox_channel_t rate_channel;
  size_t step_count;
  uint16_t steps[1];
} dmxbox_effect_t;

typedef struct dmxbox_effect_entry {
  uint16_t id;
  size_t size;
  dmxbox_effect_t *effect;
} dmxbox_effect_entry_t;

esp_err_t dmxbox_effect_get(uint16_t effect_id, dmxbox_effect_t **result);
esp_err_t dmxbox_effect_create(const dmxbox_effect_t *effect, uint16_t *id);
esp_err_t
dmxbox_effect_list(uint16_t skip, uint16_t *count, dmxbox_effect_entry_t *page);
