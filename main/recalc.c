#include <string.h>

#include "const.h"
#include "artnet/artnet.h"
#include "dmx/dmx_receive.h"
#include "recalc.h"

// static const char *TAG = "recalc";

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

void recalc(uint8_t data[DMX_PACKET_SIZE_MAX], bool *artnet_active, bool *dmx_out_active)
{
    *artnet_active = false;
    *dmx_out_active = false;

    data[0] = 0;
    memset(data, 0, DMX_PACKET_SIZE_MAX);

    if (dmx_in_connected)
    {
        taskENTER_CRITICAL(&dmx_in_spinlock);
        memcpy(data + 1, dmx_in_data + 1, DMX_CHANNEL_COUNT);
        taskEXIT_CRITICAL(&dmx_in_spinlock);
    }

    taskENTER_CRITICAL(&artnet_spinlock);
    for (int i = 1; i < DMX_PACKET_SIZE_MAX; i++)
    {
        data[i] = MAX(data[i], artnet_in_data[i - 1]);

        *artnet_active |= artnet_in_data[i - 1] != 0;
        *dmx_out_active |= data[i] != 0;
    }
    taskEXIT_CRITICAL(&artnet_spinlock);
}
