#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "const.h"
#include "artnet/artnet.h"
#include "dmx/dmx_receive.h"
#include "recalc.h"

static const char *TAG = "recalc";

#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

void recalc(uint8_t data[DMX_MAX_PACKET_SIZE], bool *artnet_active, bool *dmx_out_active)
{
    *artnet_active = false;
    *dmx_out_active = false;

    data[0] = 0;
    memset(data, 0, DMX_MAX_PACKET_SIZE);

    if (dmx_in_connected)
    {
        taskENTER_CRITICAL(&dmx_in_spinlock);
        memcpy(data + 1, dmx_in_data, DMX_CHANNEL_COUNT);
        taskEXIT_CRITICAL(&dmx_in_spinlock);
    }

    taskENTER_CRITICAL(&artnet_spinlock);
    for (int i = 1; i < DMX_MAX_PACKET_SIZE; i++)
    {
        data[i] = MAX(data[i], artnet_in_data[i - 1]);

        *artnet_active |= artnet_in_data[i - 1] != 0;
        *dmx_out_active |= data[i] != 0;
    }
    taskEXIT_CRITICAL(&artnet_spinlock);
}
