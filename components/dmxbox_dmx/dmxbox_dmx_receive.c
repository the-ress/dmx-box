#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

#include "const.h"
#include "dmxbox_const.h"
#include "dmxbox_dmx_receive.h"
#include "dmxbox_led.h"
#include "esp_dmx.h"

static const char *TAG = "dmx_receive";

portMUX_TYPE dmxbox_dmx_in_spinlock = portMUX_INITIALIZER_UNLOCKED;
uint8_t dmxbox_dmx_in_data[DMX_PACKET_SIZE_MAX];
bool dmxbox_dmx_in_connected = false;

// static uint32_t timer = 0;

static esp_err_t configure_dmx_in() {
  ESP_LOGI(TAG, "Configuring DMX IN");

  const dmx_config_t config = DMX_CONFIG_DEFAULT;
  if (!dmx_driver_install(DMX_IN_NUM, &config, NULL, 0)) {
    return ESP_FAIL;
  }

  if (!dmx_set_pin(
          DMX_IN_NUM,
          DMX_PIN_NO_CHANGE,
          dmxbox_dmx_gpio_in,
          DMX_PIN_NO_CHANGE
      )) {
    return ESP_FAIL;
  }

  return ESP_OK;
}

static void handle_packet(const dmx_packet_t *packet_info) {
  taskENTER_CRITICAL(&dmxbox_dmx_in_spinlock);
  size_t bytes_read =
      dmx_read(DMX_IN_NUM, dmxbox_dmx_in_data, packet_info->size);
  taskEXIT_CRITICAL(&dmxbox_dmx_in_spinlock);
  if (bytes_read == 0) {
    ESP_LOGE(TAG, "Unable to read DMX packet");
    return;
  }

  if (!dmxbox_dmx_in_connected) {
    dmxbox_dmx_in_connected = true;
    ESP_LOGI(TAG, "DMX connected");
    ESP_ERROR_CHECK(dmxbox_led_set(dmxbox_led_dmx_in, 1));
  }

  // timer += event->duration;

  // // print a log message every 1 second (1000000 us)
  // if (timer >= 1000000)
  // {
  //     ESP_LOGI(TAG, "DMX IN data");
  //     ESP_LOG_BUFFER_HEX(TAG, dmx_in_data, DMX_PACKET_SIZE_MAX);
  //     timer -= 1000000;
  // }
}

static void handle_timeout() {
  if (dmxbox_dmx_in_connected) {
    dmxbox_dmx_in_connected = false;
    ESP_LOGI(TAG, "DMX connection lost");
    ESP_ERROR_CHECK(dmxbox_led_set(dmxbox_led_dmx_in, 0));
  }
}

void dmxbox_dmx_receive_task(void *parameter) {
  ESP_LOGI(TAG, "DMX receive task started");

  ESP_ERROR_CHECK(configure_dmx_in());

  dmx_packet_t packet_info;

  while (1) {
    size_t packet_size = dmx_receive_num(
        DMX_IN_NUM,
        &packet_info,
        DMX_PACKET_SIZE_MAX,
        DMX_TIMEOUT_TICK
    );

    if (packet_size == 0) {
      // No packets received before timeout
      handle_timeout();
      continue;
    }

    if (packet_info.is_rdm) {
      ESP_LOGI(TAG, "Ignoring RDM packet");
      continue;
    }

    switch (packet_info.err) {
    case DMX_OK:
      handle_packet(&packet_info);
      break;

    case DMX_ERR_TIMEOUT:
      handle_timeout();
      break;

    case DMX_ERR_UART_OVERFLOW:
      ESP_LOGE(TAG, "Data could not be processed in time");
      // The UART overflowed while reading DMX data
      // this could occur if the interrupt mask is misconfigured or if the
      //  DMX ISR is constantly preempted
      break;

    case DMX_ERR_IMPROPER_SLOT:
      ESP_LOGE(
          TAG,
          "Received malformed byte at slot %i",
          (int)packet_info.size
      );
      // The UART received an improperly framed DMX slot
      // a slot in the packet is malformed - possibly a glitch due to the
      //  XLR connector? will need some more investigation
      // data can be recovered up until event.size
      break;

    case DMX_ERR_NOT_ENOUGH_SLOTS:
      ESP_LOGE(
          TAG,
          "User DMX buffer is too small - received %i slots",
          (int)packet_info.size
      );
      // The DMX packet size is smaller than expected
      // whoops - our buffer isn't big enough
      // this code will not run if buffer size is set to DMX_PACKET_SIZE_MAX
      break;

    case DMX_FAIL:
      ESP_LOGE(TAG, "Unable to receive data");
      // Generic DMX error code indicating failure
      break;
    }
  }
}

void dmxbox_dmx_receive_get_data(uint8_t data[DMX_CHANNEL_COUNT]) {
  taskENTER_CRITICAL(&dmxbox_dmx_in_spinlock);
  memcpy(data, dmxbox_dmx_in_data + 1, DMX_CHANNEL_COUNT);
  taskEXIT_CRITICAL(&dmxbox_dmx_in_spinlock);
}
