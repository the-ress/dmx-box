#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "const.h"
#include "led.h"
#include "dmx_send.h"

#define DMX_SEND_PERIOD 30

static const char *TAG = "dmx_send";

portMUX_TYPE dmx_out_spinlock = portMUX_INITIALIZER_UNLOCKED;
uint8_t dmx_out_data[DMX_MAX_PACKET_SIZE] = {0};

static void configure_dmx_out(void)
{
    ESP_LOGI(TAG, "Configuring DMX OUT");

    const dmx_config_t config = DMX_DEFAULT_CONFIG;
    ESP_ERROR_CHECK(dmx_param_config(DMX_OUT_NUM, &config));
    ESP_ERROR_CHECK(dmx_set_pin(DMX_OUT_NUM, DMX_OUT_GPIO, DMX_PIN_NO_CHANGE, DMX_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(dmx_driver_install(DMX_OUT_NUM, DMX_MAX_PACKET_SIZE, 10, NULL, ESP_INTR_FLAG_IRAM));
    ESP_ERROR_CHECK(dmx_set_mode(DMX_OUT_NUM, DMX_MODE_TX));
}

void dmx_send(void *parameter)
{
    ESP_LOGI(TAG, "DMX send task started");

    configure_dmx_out();

    while (1)
    {
        // write the packet to the DMX driver
        taskENTER_CRITICAL(&dmx_out_spinlock);
        ESP_ERROR_CHECK(dmx_write_packet(DMX_OUT_NUM, dmx_out_data, DMX_MAX_PACKET_SIZE));
        taskEXIT_CRITICAL(&dmx_out_spinlock);

        // transmit the packet on the DMX bus
        ESP_ERROR_CHECK(dmx_tx_packet(DMX_OUT_NUM));

        vTaskDelay(DMX_SEND_PERIOD / portTICK_PERIOD_MS);

        // block until the packet is done being sent
        ESP_ERROR_CHECK(dmx_wait_tx_done(DMX_OUT_NUM, DMX_TX_PACKET_TOUT_TICK));
    }
}

static bool dmx_out_active = false;

void set_dmx_out_active(bool state)
{
    if (dmx_out_active != state)
    {
        ESP_ERROR_CHECK(led_set_state(DMX_OUT_LED_GPIO, state));
        dmx_out_active = state;
    }
}
