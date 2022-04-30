#include "esp_dmx.h"

extern portMUX_TYPE dmx_in_spinlock;
extern uint8_t dmx_in_data[DMX_MAX_PACKET_SIZE];
extern bool dmx_in_connected;

void dmx_receive(void *parameter);
