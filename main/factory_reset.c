#include "const.h"
#include "dmxbox_led.h"
#include "dmxbox_storage.h"
#include <driver/gpio.h>
#include <esp_log.h>

static const char *TAG = "factory_reset";

static bool pwr_led = true;

void setup_reset_button(void) {
  ESP_ERROR_CHECK(gpio_reset_pin(RESET_BUTTON_GPIO)); // enables pullup
  ESP_ERROR_CHECK(gpio_set_direction(RESET_BUTTON_GPIO, GPIO_MODE_INPUT));
}

bool is_reset_button_pressed() { return !gpio_get_level(RESET_BUTTON_GPIO); }

bool blink_and_wait(int blink_interval, int wait_time) {
  for (int i = 0; i < wait_time / blink_interval; i++) {
    ESP_ERROR_CHECK(dmxbox_led_set(dmxbox_led_power, pwr_led));
    vTaskDelay(blink_interval / portTICK_PERIOD_MS);
    pwr_led = !pwr_led;

    if (!is_reset_button_pressed()) {
      ESP_LOGI(TAG, "reset button released, continuing with boot");
      return false;
    }
  }

  return true;
}

void perform_factory_reset(void) {
  dmxbox_storage_factory_reset();
  ESP_LOGI(TAG, "performing factory reset");
  esp_restart();
}

void dmxbox_handle_factory_reset(void) {
  setup_reset_button();
  if (!is_reset_button_pressed()) {
    return;
  }

  if (esp_reset_reason() == ESP_RST_SW) {
    ESP_LOGI(TAG, "ignoring pressed reset button during boot after sw reset");
    return;
  }

  ESP_LOGI(
      TAG,
      "reset button pressed during boot, waiting 10 seconds before factory "
      "reset");

  if (!blink_and_wait(250, 8000)) {
    return;
  }

  // faster blinking for the last 2 seconds
  if (!blink_and_wait(100, 2000)) {
    return;
  }

  perform_factory_reset();
}
