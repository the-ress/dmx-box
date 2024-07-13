idf_component_register(
  SRCS dmxbox_dns.c
  INCLUDE_DIRS include
  REQUIRES
    dmxbox_storage
    esp_netif
)
