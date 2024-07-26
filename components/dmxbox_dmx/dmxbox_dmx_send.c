#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

#include "const.h"
#include "dmxbox_const.h"
#include "dmxbox_dmx_send.h"
#include "dmxbox_led.h"
#include "esp_dmx.h"

#define DMX_SEND_PERIOD 30
#define DMX_SEND_BACKOFF 200

static const char *TAG = "dmx_send";

portMUX_TYPE dmxbox_dmx_out_spinlock = portMUX_INITIALIZER_UNLOCKED;
uint8_t dmxbox_dmx_out_data[DMX_PACKET_SIZE_MAX] = {0};

static esp_err_t configure_dmx_out() {
  ESP_LOGI(TAG, "Configuring DMX OUT");

  const dmx_config_t config = DMX_CONFIG_DEFAULT;
  if (!dmx_driver_install(DMX_OUT_NUM, &config, NULL, 0)) {
    return ESP_FAIL;
  }

  if (!dmx_set_pin(
          DMX_OUT_NUM,
          dmxbox_dmx_gpio_out,
          DMX_PIN_NO_CHANGE,
          DMX_PIN_NO_CHANGE
      )) {
    return ESP_FAIL;
  }

  return ESP_OK;
}

void dmxbox_dmx_send_task(void *parameter) {
  ESP_LOGI(TAG, "DMX send task started");

  ESP_ERROR_CHECK(configure_dmx_out());

  TickType_t xLastWakeTime = xTaskGetTickCount();
  while (1) {
    // write the packet to the DMX driver
    taskENTER_CRITICAL(&dmxbox_dmx_out_spinlock);
    size_t bytes_written =
        dmx_write(DMX_OUT_NUM, dmxbox_dmx_out_data, DMX_PACKET_SIZE_MAX);
    taskEXIT_CRITICAL(&dmxbox_dmx_out_spinlock);

    if (bytes_written == 0) {
      ESP_LOGE(TAG, "Unable to write DMX data");
      vTaskDelay(DMX_SEND_BACKOFF / portTICK_PERIOD_MS);
      continue;
    }

    // transmit the packet on the DMX bus
    size_t bytes_sent = dmx_send(DMX_OUT_NUM);
    if (bytes_sent == 0) {
      ESP_LOGE(TAG, "Unable to send DMX data");
      vTaskDelay(DMX_SEND_BACKOFF / portTICK_PERIOD_MS);
      continue;
    }

    vTaskDelayUntil(&xLastWakeTime, DMX_SEND_PERIOD / portTICK_PERIOD_MS);

    // block until the packet is done being sent
    if (!dmx_wait_sent(DMX_OUT_NUM, DMX_TIMEOUT_TICK)) {
      ESP_LOGE(TAG, "DMX send timed out");
      vTaskDelay(DMX_SEND_BACKOFF / portTICK_PERIOD_MS);
      continue;
    }
  }
}

static bool dmx_out_active = false;

void dmxbox_set_dmx_out_active(bool state) {
  if (dmx_out_active != state) {
    ESP_ERROR_CHECK(dmxbox_led_set(dmxbox_led_dmx_out, state));
    dmx_out_active = state;
  }
}

void dmxbox_dmx_send_get_data(uint8_t data[DMX_CHANNEL_COUNT]) {
  taskENTER_CRITICAL(&dmxbox_dmx_out_spinlock);
  memcpy(data, dmxbox_dmx_out_data + 1, DMX_CHANNEL_COUNT);
  taskEXIT_CRITICAL(&dmxbox_dmx_out_spinlock);
}
