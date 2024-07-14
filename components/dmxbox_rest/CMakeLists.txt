idf_component_register(
  SRCS 
    dmxbox_rest.c
    dmxbox_uri.c
  INCLUDE_DIRS include
  REQUIRES dmxbox_httpd esp_http_server json
)

