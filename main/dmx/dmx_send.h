#include "esp_dmx.h"

extern portMUX_TYPE dmx_out_spinlock;
extern uint8_t dmx_out_data[DMX_MAX_PACKET_SIZE];

void dmx_send(void *parameter);
void set_dmx_out_active(bool state);
