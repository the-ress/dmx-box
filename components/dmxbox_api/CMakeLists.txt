idf_component_register(
  SRCS
    api_config.c
    api_strings.c
    dmxbox_api.c
    settings_sta.c
    ws.c
    ws_ap_found.c
  INCLUDE_DIRS include
  REQUIRES 
    dmxbox_storage
    dmxbox_wifi
    dmxbox_httpd
    esp_http_server
    json
    vfs
)


