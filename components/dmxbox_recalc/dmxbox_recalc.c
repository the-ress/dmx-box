#include <esp_log.h>
#include <string.h>

#include "dmxbox_artnet.h"
#include "dmxbox_const.h"
#include "dmxbox_dmx_receive.h"
#include "dmxbox_dmx_send.h"
#include "dmxbox_effects.h"
#include "dmxbox_led.h"
#include "dmxbox_recalc.h"

static const char *TAG = "recalc";

#define CONFIG_RECALC_PERIOD 30

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

static void dmxbox_recalc(
    uint8_t data[DMX_PACKET_SIZE_MAX],
    bool *artnet_active,
    bool *dmx_out_active
) {
  *artnet_active = false;
  *dmx_out_active = false;

  data[0] = 0;
  memset(data, 0, DMX_PACKET_SIZE_MAX);

  if (dmxbox_dmx_in_connected) {
    taskENTER_CRITICAL(&dmxbox_dmx_in_spinlock);
    memcpy(data + 1, dmxbox_dmx_in_data + 1, DMX_CHANNEL_COUNT);
    taskEXIT_CRITICAL(&dmxbox_dmx_in_spinlock);
  }

  taskENTER_CRITICAL(&dmxbox_artnet_spinlock);
  const uint8_t *artnet_data = dmxbox_artnet_get_native_universe_data();
  for (uint16_t i = 1; i < DMX_PACKET_SIZE_MAX; i++) {
    data[i] = MAX(MAX(data[i], artnet_data[i - 1]), dmxbox_effects_data[i - 1]);

    *artnet_active |= artnet_data[i - 1] != 0;
    *dmx_out_active |= data[i] != 0;
  }
  taskEXIT_CRITICAL(&dmxbox_artnet_spinlock);
}

void dmxbox_recalc_task(void *parameter) {
  ESP_LOGI(TAG, "Recalc task started");

  TickType_t xLastWakeTime = xTaskGetTickCount();

  uint8_t data[DMX_PACKET_SIZE_MAX];
  while (1) {
    bool artnet_active;
    bool dmx_out_active;

    dmxbox_recalc(data, &artnet_active, &dmx_out_active);

    taskENTER_CRITICAL(&dmxbox_dmx_out_spinlock);
    memcpy(dmxbox_dmx_out_data, data, DMX_PACKET_SIZE_MAX);
    taskEXIT_CRITICAL(&dmxbox_dmx_out_spinlock);

    dmxbox_set_artnet_active(artnet_active);
    dmxbox_set_dmx_out_active(dmx_out_active);

    vTaskDelayUntil(&xLastWakeTime, CONFIG_RECALC_PERIOD / portTICK_PERIOD_MS);
  }
}
