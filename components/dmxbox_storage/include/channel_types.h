#pragma once
#include <stdint.h>
#include "universe_storage.h"

typedef uint16_t dmxbox_channel_index_t;

typedef struct dmxbox_channel {
  dmxbox_universe_t universe;
  dmxbox_channel_index_t index;
} dmxbox_channel_t;
