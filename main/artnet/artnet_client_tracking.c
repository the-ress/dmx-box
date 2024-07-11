#include "lwip/sockets.h"
#include <esp_log.h>
#include <stdint.h>

#include "const.h"
#include "hashmap.h"

static const char *TAG = "artnet_client_tracking";

typedef struct dmxbox_artnet_client_tracking_universe {
  struct dmxbox_artnet_client_tracking_universe *next;
  uint16_t address;
  uint8_t data[DMX_CHANNEL_COUNT];
} dmxbox_artnet_client_tracking_universe_t;

static dmxbox_artnet_client_tracking_universe_t *
dmxbox_artnet_client_tracking_universe_alloc() {
  return calloc(1, sizeof(dmxbox_artnet_client_tracking_universe_t));
}

static void dmxbox_artnet_client_tracking_universe_free(
    dmxbox_artnet_client_tracking_universe_t *data
) {
  free(data);
}

typedef struct dmxbox_artnet_client_tracking_entry {
  struct sockaddr_storage addr;
  dmxbox_artnet_client_tracking_universe_t *last_data_head;
} dmxbox_artnet_client_tracking_entry_t;

portMUX_TYPE client_map_spinlock = portMUX_INITIALIZER_UNLOCKED;
static struct hashmap *client_map;

static void free_client_map_entry(void *item) {
  dmxbox_artnet_client_tracking_entry_t *entry = item;

  dmxbox_artnet_client_tracking_universe_t *universe = entry->last_data_head;
  while (universe) {
    dmxbox_artnet_client_tracking_universe_t *current_universe = universe;
    universe = universe->next;
    dmxbox_artnet_client_tracking_universe_free(current_universe);
  }
  entry->last_data_head = NULL;
}

static int ip_compare(const void *a, const void *b, void *udata) {
  const dmxbox_artnet_client_tracking_entry_t *entrya = a;
  const dmxbox_artnet_client_tracking_entry_t *entryb = b;

  if (entrya->addr.ss_family != entryb->addr.ss_family) {
    return entrya->addr.ss_family - entryb->addr.ss_family;
  }

  switch (entrya->addr.ss_family) {
  case PF_INET: {
    struct in_addr ipa = ((struct sockaddr_in *)&entrya->addr)->sin_addr;
    struct in_addr ipb = ((struct sockaddr_in *)&entryb->addr)->sin_addr;
    return memcmp(&ipa, &ipb, sizeof(ipa));
  }
  case PF_INET6: {
    struct in6_addr ipa = ((struct sockaddr_in6 *)&entrya->addr)->sin6_addr;
    struct in6_addr ipb = ((struct sockaddr_in6 *)&entryb->addr)->sin6_addr;
    return memcmp(&ipa, &ipb, sizeof(ipa));
  }
  }

  ESP_LOGE(TAG, "Unexpected address family: %d", entrya->addr.ss_family);
  return 0;
}

static uint64_t ip_hash(const void *item, uint64_t seed0, uint64_t seed1) {
  const dmxbox_artnet_client_tracking_entry_t *entry = item;

  switch (entry->addr.ss_family) {
  case PF_INET: {
    struct in_addr ip = ((struct sockaddr_in *)&entry->addr)->sin_addr;
    return hashmap_sip(&ip, sizeof(ip), seed0, seed1);
  }
  case PF_INET6: {
    struct in6_addr ip = ((struct sockaddr_in6 *)&entry->addr)->sin6_addr;
    return hashmap_sip(&ip, sizeof(ip), seed0, seed1);
  }
  }

  ESP_LOGE(TAG, "Unexpected address family: %d", entry->addr.ss_family);
  return 0;
}

static dmxbox_artnet_client_tracking_universe_t *find_universe_record(
    const dmxbox_artnet_client_tracking_entry_t *entry,
    uint16_t address
) {
  for (dmxbox_artnet_client_tracking_universe_t *client_universe =
           entry->last_data_head;
       client_universe;
       client_universe = client_universe->next) {
    if (client_universe->address == address) {
      return client_universe;
    }
  }

  return NULL;
}

static dmxbox_artnet_client_tracking_universe_t *
add_universe_record(dmxbox_artnet_client_tracking_entry_t *entry) {
  dmxbox_artnet_client_tracking_universe_t *universe_record =
      dmxbox_artnet_client_tracking_universe_alloc();

  universe_record->next = entry->last_data_head;
  entry->last_data_head = universe_record;

  return universe_record;
}

void dmxbox_artnet_client_tracking_initialize() {
  client_map = hashmap_new(
      sizeof(dmxbox_artnet_client_tracking_entry_t),
      0,
      0,
      0,
      ip_hash,
      ip_compare,
      free_client_map_entry,
      NULL
  );
}

void dmxbox_artnet_client_tracking_reset() {
  taskENTER_CRITICAL(&client_map_spinlock);
  hashmap_clear(client_map, true);
  taskEXIT_CRITICAL(&client_map_spinlock);
}

bool dmxbox_artnet_client_tracking_get_last_data(
    const struct sockaddr_storage *source_addr,
    uint16_t universe_address,
    uint8_t *data
) {
  bool result = false;

  const dmxbox_artnet_client_tracking_entry_t key = {.addr = *source_addr};

  taskENTER_CRITICAL(&client_map_spinlock);
  const dmxbox_artnet_client_tracking_entry_t *entry =
      hashmap_get(client_map, &key);

  if (!entry) {
    goto exit;
  }

  const dmxbox_artnet_client_tracking_universe_t *universe_record =
      find_universe_record(entry, universe_address);

  if (!universe_record) {
    goto exit;
  }

  result = true;
  memcpy(data, universe_record->data, DMX_CHANNEL_COUNT);

exit:
  taskEXIT_CRITICAL(&client_map_spinlock);

  return result;
}

void dmxbox_artnet_client_tracking_set_last_data(
    const struct sockaddr_storage *source_addr,
    uint16_t universe_address,
    const uint8_t *data,
    uint16_t data_length
) {
  const dmxbox_artnet_client_tracking_entry_t key = {.addr = *source_addr};

  taskENTER_CRITICAL(&client_map_spinlock);
  dmxbox_artnet_client_tracking_entry_t *previous_entry =
      hashmap_get(client_map, &key);

  dmxbox_artnet_client_tracking_entry_t new_entry = {
      .addr = *source_addr,
      .last_data_head = previous_entry ? previous_entry->last_data_head : NULL,
  };

  dmxbox_artnet_client_tracking_universe_t *universe_record =
      find_universe_record(&new_entry, universe_address);

  if (!universe_record) {
    universe_record = add_universe_record(&new_entry);
    universe_record->address = universe_address;

    hashmap_set(client_map, &new_entry);
  }

  if (data_length != DMX_CHANNEL_COUNT) {
    memset(universe_record->data, 0, DMX_CHANNEL_COUNT);
  }
  memcpy(universe_record->data, data, data_length);
  taskEXIT_CRITICAL(&client_map_spinlock);
}
