#include "driver/gpio.h"

void dmxbox_configure_leds(void);
esp_err_t dmxbox_led_set_state(gpio_num_t gpio_num, uint32_t level);
