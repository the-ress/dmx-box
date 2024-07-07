idf_component_register(
    SRCS
        "artnet/artnet.c"
        "dmx/dmx_receive.c"
        "dmx/dmx_send.c"
        "dns.c"
        "factory_reset.c"
        "hashmap.c"
        "led.c"
        "main.c"
        "recalc.c"
        "storage.c"
        "webserver/webserver.c"
        "webserver/static_files.c"
        "webserver/wifi_scan.c"
        "wifi.c"
    INCLUDE_DIRS "."
)

set(WEB_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../dmxui")
if(EXISTS ${WEB_SRC_DIR}/dist)
    spiffs_create_partition_image(www ${WEB_SRC_DIR}/dist FLASH_IN_PROJECT)
else()
    message(FATAL_ERROR "${WEB_SRC_DIR}/dist doesn't exit. Please run 'make' in ${WEB_SRC_DIR}")
endif()
