#include "const.h"

extern portMUX_TYPE artnet_spinlock;
extern uint8_t artnet_in_data[DMX_CHANNEL_COUNT];

void artnet_initialize(void);

void artnet_receive_task(void *parameter);
void set_artnet_active(bool state);
