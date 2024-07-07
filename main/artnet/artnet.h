#include "const.h"

extern portMUX_TYPE dmxbox_artnet_spinlock;
extern uint8_t dmxbox_artnet_in_data[DMX_CHANNEL_COUNT];

void dmxbox_artnet_initialize(void);

void dmxbox_artnet_receive_task(void *parameter);
void dmxbox_set_artnet_active(bool state);
