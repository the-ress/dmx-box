#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <portmacro.h>

#include "dmxbox_led.h"

static const char *TAG = "dmxbox_led";

static void dmxbox_led_start_one(dmxbox_led_t led, bool level) {
  ESP_ERROR_CHECK(gpio_set_level(led, !level));
  ESP_ERROR_CHECK(gpio_reset_pin(led));
  ESP_ERROR_CHECK(gpio_set_direction(led, GPIO_MODE_OUTPUT));
}

void dmxbox_led_start() {
  ESP_LOGI(TAG, "Configuring LED GPIOs");
  dmxbox_led_start_one(dmxbox_led_dmx_out, 1);
  dmxbox_led_start_one(dmxbox_led_power, 1);
  dmxbox_led_start_one(dmxbox_led_dmx_in, 1);
  dmxbox_led_start_one(dmxbox_led_artnet_in, 1);
  dmxbox_led_start_one(dmxbox_led_ap, 1);
  dmxbox_led_start_one(dmxbox_led_sta, 1);

  vTaskDelay(50 / portTICK_PERIOD_MS);

  dmxbox_led_set(dmxbox_led_dmx_out, 0);
  dmxbox_led_set(dmxbox_led_power, 0);
  dmxbox_led_set(dmxbox_led_dmx_in, 0);
  dmxbox_led_set(dmxbox_led_artnet_in, 0);
  dmxbox_led_set(dmxbox_led_ap, 0);
  dmxbox_led_set(dmxbox_led_sta, 0);
}

esp_err_t dmxbox_led_set(dmxbox_led_t led, bool level) {
  return gpio_set_level(led, !level);
}
