#include <string.h>

#include "artnet/artnet.h"
#include "const.h"
#include "dmx/dmx_receive.h"
#include "recalc.h"

// static const char *TAG = "recalc";

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

void dmxbox_recalc(
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
  for (int i = 1; i < DMX_PACKET_SIZE_MAX; i++) {
    data[i] = MAX(data[i], dmxbox_artnet_in_data[i - 1]);

    *artnet_active |= dmxbox_artnet_in_data[i - 1] != 0;
    *dmx_out_active |= data[i] != 0;
  }
  taskEXIT_CRITICAL(&dmxbox_artnet_spinlock);
}
