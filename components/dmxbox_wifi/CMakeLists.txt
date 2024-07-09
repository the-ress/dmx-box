idf_component_register(
  SRCS
    wifi.c
    wifi_scan.c
  INCLUDE_DIRS . ../../main # TODO FIXME
  REQUIRES 
    esp_wifi
    esp_dmx # REMOVE
)

