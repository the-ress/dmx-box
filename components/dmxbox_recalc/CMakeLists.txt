idf_component_register(
  SRCS dmxbox_recalc.c
  INCLUDE_DIRS include
  REQUIRES
    dmxbox_artnet
    dmxbox_dmx
    dmxbox_effects
    dmxbox_led
)
