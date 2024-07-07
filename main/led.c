#include "led.h"
#include "const.h"
#include "esp_log.h"

static const char *TAG = "led";

void dmxbox_configure_leds(void) {
  ESP_LOGI(TAG, "Configuring LED GPIOs");

  ESP_ERROR_CHECK(dmxbox_led_set_state(POWER_LED_GPIO, 0));
  ESP_ERROR_CHECK(dmxbox_led_set_state(DMX_OUT_LED_GPIO, 0));
  ESP_ERROR_CHECK(dmxbox_led_set_state(DMX_IN_LED_GPIO, 0));
  ESP_ERROR_CHECK(dmxbox_led_set_state(ARTNET_IN_LED_GPIO, 0));
  ESP_ERROR_CHECK(dmxbox_led_set_state(AP_MODE_LED_GPIO, 0));
  ESP_ERROR_CHECK(dmxbox_led_set_state(CLIENT_MODE_LED_GPIO, 0));

  ESP_ERROR_CHECK(gpio_reset_pin(POWER_LED_GPIO));
  ESP_ERROR_CHECK(gpio_reset_pin(DMX_OUT_LED_GPIO));
  ESP_ERROR_CHECK(gpio_reset_pin(DMX_IN_LED_GPIO));
  ESP_ERROR_CHECK(gpio_reset_pin(ARTNET_IN_LED_GPIO));
  ESP_ERROR_CHECK(gpio_reset_pin(AP_MODE_LED_GPIO));
  ESP_ERROR_CHECK(gpio_reset_pin(CLIENT_MODE_LED_GPIO));

  ESP_ERROR_CHECK(gpio_set_direction(POWER_LED_GPIO, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_direction(DMX_OUT_LED_GPIO, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_direction(DMX_IN_LED_GPIO, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_direction(ARTNET_IN_LED_GPIO, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_direction(AP_MODE_LED_GPIO, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_direction(CLIENT_MODE_LED_GPIO, GPIO_MODE_OUTPUT));
}

esp_err_t dmxbox_led_set_state(gpio_num_t gpio_num, uint32_t level) {
  return gpio_set_level(gpio_num, !level);
}
