#include "driver/gpio.h"

void configure_leds(void);
esp_err_t led_set_state(gpio_num_t gpio_num, uint32_t level);
