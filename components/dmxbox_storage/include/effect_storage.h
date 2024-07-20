#pragma once
#include "channel_types.h"
#include "entry.h"
#include <esp_err.h>
#include <stddef.h>

typedef struct dmxbox_effect {
  char name[33];
  dmxbox_channel_t level_channel;
  dmxbox_channel_t rate_channel;
  uint16_t distributed_id;
  size_t step_count;
  uint16_t steps[1];
} __attribute__((packed)) dmxbox_effect_t;

dmxbox_effect_t *dmxbox_effect_alloc(size_t step_count);

esp_err_t dmxbox_effect_get(uint16_t effect_id, dmxbox_effect_t **result);
esp_err_t dmxbox_effect_set(uint16_t effect_id, const dmxbox_effect_t *effect);
esp_err_t dmxbox_effect_create(const dmxbox_effect_t *effect, uint16_t *id);
esp_err_t dmxbox_effect_delete(uint16_t effect_id);
esp_err_t dmxbox_effect_list(
    uint16_t skip,
    uint16_t *count,
    dmxbox_storage_entry_t *page
);
