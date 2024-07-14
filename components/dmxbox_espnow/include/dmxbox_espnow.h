#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef void (*dmxbox_espnow_effect_state_callback_t)(
    uint16_t effect_id,
    uint8_t level,
    uint8_t rate_raw,
    double progress,
    bool first_pass
);

void dmxbox_espnow_init();

void dmxbox_espnow_register_effect_state_callback(
    dmxbox_espnow_effect_state_callback_t cb
);

void dmxbox_espnow_send_effect_state(
    uint16_t effect_id,
    uint8_t level,
    uint8_t rate_raw,
    double progress,
    bool first_pass
);
