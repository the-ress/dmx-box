#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct dmxbox_storage_entry_t {
  uint16_t id;
  size_t size;
  void *data;

} dmxbox_storage_entry_t;
