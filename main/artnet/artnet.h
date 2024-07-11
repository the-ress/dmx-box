#include "const.h"

extern portMUX_TYPE dmxbox_artnet_spinlock;
const uint8_t *dmxbox_artnet_get_native_universe_data();
const uint8_t *dmxbox_artnet_get_universe_data(uint16_t address);

void dmxbox_artnet_initialize(void);

void dmxbox_artnet_receive_task(void *parameter);
void dmxbox_set_artnet_active(bool state);
