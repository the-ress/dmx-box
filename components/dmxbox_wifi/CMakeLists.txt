idf_component_register(
  SRCS
    wifi.c
    wifi_scan.c
  INCLUDE_DIRS include
  REQUIRES
    dmxbox_led
    dmxbox_storage
    esp_wifi
)

