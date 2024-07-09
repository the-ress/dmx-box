idf_component_register(
  SRCS
    api_config.c
    api_config.h
    api_cors.c
    api_cors.h
    ui.c
    ui.h
    ws.c
    ws.h
    ws_ap_found.c
    ws_ap_found.h
    webserver.c
  INCLUDE_DIRS include
  REQUIRES json esp_http_server dmxbox_wifi vfs
)

