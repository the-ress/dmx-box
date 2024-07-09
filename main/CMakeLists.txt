idf_component_register(
    SRCS
        "artnet/artnet.c"
        "dmx/dmx_receive.c"
        "dmx/dmx_send.c"
        "dns.c"
        "effects/effects.c"
        "factory_reset.c"
        "hashmap.c"
        "led.c"
        "main.c"
        "recalc.c"
        "storage.c"
    INCLUDE_DIRS "."
    REQUIRES dmxbox_httpd dmxbox_wifi esp_dmx esp32-button spiffs
)
