#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "const.h"
#include "led.h"
#include "dmx_receive.h"

static const char *TAG = "dmx_receive";

portMUX_TYPE dmx_in_spinlock = portMUX_INITIALIZER_UNLOCKED;
uint8_t dmx_in_data[DMX_MAX_PACKET_SIZE];
bool dmx_in_connected = false;

static uint32_t timer = 0;

static void configure_dmx_in(QueueHandle_t *dmx_queue)
{
    ESP_LOGI(TAG, "Configuring DMX IN");

    const dmx_config_t config = DMX_DEFAULT_CONFIG;
    ESP_ERROR_CHECK(dmx_param_config(DMX_IN_NUM, &config));
    ESP_ERROR_CHECK(dmx_set_pin(DMX_IN_NUM, DMX_PIN_NO_CHANGE, DMX_IN_GPIO, DMX_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(dmx_driver_install(DMX_IN_NUM, DMX_MAX_PACKET_SIZE, 10, dmx_queue, ESP_INTR_FLAG_IRAM));
}

static void handle_packet(const dmx_event_t *event)
{
    taskENTER_CRITICAL(&dmx_in_spinlock);
    ESP_ERROR_CHECK(dmx_read_packet(DMX_IN_NUM, dmx_in_data, event->size));
    taskEXIT_CRITICAL(&dmx_in_spinlock);

    if (!dmx_in_connected)
    {
        dmx_in_connected = true;
        ESP_LOGI(TAG, "DMX connected");
        ESP_ERROR_CHECK(led_set_state(DMX_IN_LED_GPIO, 1));
    }

    timer += event->duration;

    // print a log message every 1 second (1000000 us)
    if (timer >= 1000000)
    {
        ESP_LOG_BUFFER_HEX(TAG, dmx_in_data, 16);
        timer -= 1000000;
    }
}

static void handle_timeout(void)
{
    if (dmx_in_connected)
    {
        dmx_in_connected = false;
        ESP_LOGI(TAG, "DMX connection lost");
        ESP_ERROR_CHECK(led_set_state(DMX_IN_LED_GPIO, 0));
    }
}

void dmx_receive(void *parameter)
{
    ESP_LOGI(TAG, "DMX receive task started");

    QueueHandle_t dmx_queue;
    configure_dmx_in(&dmx_queue);

    dmx_event_t event;

    while (1)
    {
        if (!xQueueReceive(dmx_queue, &event, DMX_RX_PACKET_TOUT_TICK))
        {
            handle_timeout();
            continue;
        }

        switch (event.status)
        {
        case DMX_OK:
            handle_packet(&event);
            break;

        case DMX_ERR_IMPROPER_SLOT:
            ESP_LOGE(TAG, "Received malformed byte at slot %i", event.size);
            // a slot in the packet is malformed - possibly a glitch due to the
            //  XLR connector? will need some more investigation
            // data can be recovered up until event.size
            break;

        case DMX_ERR_PACKET_SIZE:
            ESP_LOGE(TAG, "Packet size %i is invalid", event.size);
            // the host DMX device is sending a bigger packet than it should
            // data may be recoverable but something went very wrong to get here
            break;

        case DMX_ERR_BUFFER_SIZE:
            ESP_LOGE(TAG, "User DMX buffer is too small - received %i slots", event.size);
            // whoops - our buffer isn't big enough
            // this code will not run if buffer size is set to DMX_MAX_PACKET_SIZE
            break;

        case DMX_ERR_DATA_OVERFLOW:
            ESP_LOGE(TAG, "Data could not be processed in time");
            // the UART FIFO overflowed
            // this could occur if the interrupt mask is misconfigured or if the
            //  DMX ISR is constantly preempted
            break;
        }
    }
}
