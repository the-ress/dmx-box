#pragma once
#include <cJSON.h>
#include <esp_http_server.h>
#include <stdint.h>

typedef struct dmxbox_rest_result {
  httpd_err_code_t error_code;
  cJSON *body;
  char location[48];
} dmxbox_rest_result_t;

typedef dmxbox_rest_result_t (*dmxbox_rest_delete_t)(
    httpd_req_t *req,
    uint32_t id
);
typedef dmxbox_rest_result_t (*dmxbox_rest_get_t)(
    httpd_req_t *req,
    uint32_t id
);
typedef dmxbox_rest_result_t (*dmxbox_rest_list_t)(httpd_req_t *req);
typedef dmxbox_rest_result_t (*dmxbox_rest_post_t)(
    httpd_req_t *req,
    cJSON *body
);
typedef dmxbox_rest_result_t (*dmxbox_rest_put_t)(
    httpd_req_t *req,
    uint32_t id,
    cJSON *body
);

struct dmxbox_rest_child_container;

typedef struct dmxbox_rest_container {
  const char* slug;
  dmxbox_rest_delete_t delete;
  dmxbox_rest_get_t get;
  dmxbox_rest_list_t list;
  dmxbox_rest_post_t post;
  dmxbox_rest_put_t put;
  const struct dmxbox_rest_child_container *first_child;
} dmxbox_rest_container_t;

typedef struct dmxbox_rest_child_container {
  const struct dmxbox_rest_child_container *sibling;
  struct dmxbox_rest_container container;
} dmxbox_rest_child_container_t;

typedef struct dmxbox_rest_uri {
  const dmxbox_rest_container_t *container;
  uint16_t parent_id;
  uint16_t child_id;
} dmxbox_rest_uri_t;

esp_err_t dmxbox_rest_parse_uri(
    const dmxbox_rest_container_t *container,
    httpd_req_t *req,
    dmxbox_rest_uri_t *result
);
