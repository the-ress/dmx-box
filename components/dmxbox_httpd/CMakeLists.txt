idf_component_register(
  SRCS
    api_config.c
    api_cors.c
    ui.c
    ws.c
    ws_ap_found.c
    webserver.c
  INCLUDE_DIRS include
  REQUIRES 
    dmxbox_storage
    dmxbox_wifi
    esp_http_server
    json
    vfs
)

