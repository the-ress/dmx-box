#include "lwip/sockets.h"
#include <stdint.h>

void dmxbox_artnet_client_tracking_initialize();
void dmxbox_artnet_client_tracking_reset();

bool dmxbox_artnet_client_tracking_get_last_data(
    const struct sockaddr_storage *source_addr,
    uint16_t universe_address,
    uint8_t *data
);

void dmxbox_artnet_client_tracking_set_last_data(
    const struct sockaddr_storage *source_addr,
    uint16_t universe_address,
    const uint8_t *data,
    uint16_t data_length
);
