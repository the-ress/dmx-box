#pragma once

typedef union dmxbox_storage_universe {
  unsigned address : 15;
  struct {
    unsigned net : 7;
    unsigned subnet : 4;
    unsigned universe : 4;
  };
} dmxbox_storage_universe_t;

