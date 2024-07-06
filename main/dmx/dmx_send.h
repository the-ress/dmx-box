#include "esp_dmx.h"

extern portMUX_TYPE dmx_out_spinlock;
extern uint8_t dmx_out_data[DMX_PACKET_SIZE_MAX];

void dmx_send_task(void *parameter);
void set_dmx_out_active(bool state);
