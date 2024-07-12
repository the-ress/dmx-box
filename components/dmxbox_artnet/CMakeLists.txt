idf_component_register(
  SRCS
    artnet_client_tracking.c
    dmxbox_artnet.c
    hashmap.c
  INCLUDE_DIRS include
  REQUIRES
    dmxbox_const
    dmxbox_led
    dmxbox_storage
    dmxbox_wifi
    esp_netif
    esp32-button
)
