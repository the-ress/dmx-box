#pragma once
#include <esp_err.h>
#include <stdbool.h>

typedef enum dmxbox_led {
  dmxbox_led_dmx_out = 14,
  dmxbox_led_power = 18,
  dmxbox_led_dmx_in = 19,
  dmxbox_led_artnet_in = 21,
  dmxbox_led_ap = 22,
  dmxbox_led_sta = 27,
} dmxbox_led_t;

void dmxbox_led_start(void);
esp_err_t dmxbox_led_set(dmxbox_led_t, bool level);
