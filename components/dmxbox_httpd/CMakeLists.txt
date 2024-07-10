idf_component_register(
  SRCS
    private/api_strings.c
    private/cors.c
    private/receive_json.c
    private/scratch.c
    settings_sta.c
    api_config.c
    statics.c
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

