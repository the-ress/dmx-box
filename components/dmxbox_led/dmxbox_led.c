#include "dmxbox_led.h"
#include <driver/gpio.h>
#include <esp_log.h>

static const char *TAG = "dmxbox_led";

static void dmxbox_led_start_one(dmxbox_led_t led) {
  ESP_ERROR_CHECK(gpio_set_level(led, 1));
  ESP_ERROR_CHECK(gpio_reset_pin(led));
  ESP_ERROR_CHECK(gpio_set_direction(led, GPIO_MODE_OUTPUT));
}

void dmxbox_led_start(void) {
  ESP_LOGI(TAG, "Configuring LED GPIOs");
  dmxbox_led_start_one(dmxbox_led_dmx_out);
  dmxbox_led_start_one(dmxbox_led_power);
  dmxbox_led_start_one(dmxbox_led_dmx_in);
  dmxbox_led_start_one(dmxbox_led_artnet_in);
  dmxbox_led_start_one(dmxbox_led_ap);
  dmxbox_led_start_one(dmxbox_led_sta);
}

esp_err_t dmxbox_led_set(dmxbox_led_t led, bool level) {
  return gpio_set_level(led, !level);
}
