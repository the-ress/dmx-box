idf_component_register(
  SRCS
    cors.c
    json.c
    scratch.c
    statics.c
  INCLUDE_DIRS include
  REQUIRES
    esp_http_server
    json
    vfs
)
