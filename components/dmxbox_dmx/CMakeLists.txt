idf_component_register(
  SRCS
    dmxbox_dmx_receive.c
    dmxbox_dmx_send.c
  INCLUDE_DIRS include
  REQUIRES
    dmxbox_const
    dmxbox_led
    esp_dmx
)
