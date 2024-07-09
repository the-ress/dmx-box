idf_component_register(
  SRCS
    api/api_strings.c
    api/settings_sta.c
    api_config.c
    api_cors.c
    receive_json.c
    scratch.c
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

