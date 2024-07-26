#pragma once
#include <cJSON.h>

#include "dmxbox_rest.h"
#include "effect_step_storage.h"
#include "serializer.h"

DMXBOX_API_SERIALIZER_HEADERS(dmxbox_channel_level_t, channel_level);
extern const dmxbox_rest_container_t effects_steps_router;
