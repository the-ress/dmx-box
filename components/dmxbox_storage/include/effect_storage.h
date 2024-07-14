#pragma once
#include "channel_types.h"
#include <esp_err.h>
#include <stddef.h>

typedef struct dmxbox_effect {
  dmxbox_channel_t level_channel;
  dmxbox_channel_t rate_channel;
  size_t step_count;
} dmxbox_effect_t;

esp_err_t dmxbox_effect_get(uint16_t effect_id, dmxbox_effect_t **result);
