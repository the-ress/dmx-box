#pragma once
#include <cJSON.h>
#include <esp_http_server.h>
#include <stdint.h>

typedef struct dmxbox_rest_result {
  const char *status;
  cJSON *body;
  char location[48];
} dmxbox_rest_result_t;

extern const dmxbox_rest_result_t dmxbox_rest_200_ok;
extern const dmxbox_rest_result_t dmxbox_rest_204_no_content;
dmxbox_rest_result_t dmxbox_rest_400_bad_request(const char *message);
dmxbox_rest_result_t dmxbox_rest_404_not_found(const char *message);
extern const dmxbox_rest_result_t dmxbox_rest_405_method_not_allowed;
dmxbox_rest_result_t dmxbox_rest_500_internal_server_error(const char *message);

dmxbox_rest_result_t dmxbox_rest_201_created(const char *location_format, ...);
dmxbox_rest_result_t dmxbox_rest_result_json(cJSON *);

struct dmxbox_rest_container;
struct dmxbox_rest_child_container;

typedef struct dmxbox_rest_uri {
  const struct dmxbox_rest_container *container;
  uint16_t parent_id;
  uint16_t id;
  bool has_id;
} dmxbox_rest_uri_t;

typedef dmxbox_rest_result_t (*dmxbox_rest_delete_t)(
    httpd_req_t *req,
    uint16_t parent_id,
    uint16_t child_id
);
typedef dmxbox_rest_result_t (*dmxbox_rest_get_t)(
    httpd_req_t *req,
    uint16_t parent_id,
    uint16_t child_id
);
typedef dmxbox_rest_result_t (*dmxbox_rest_list_t)(
    httpd_req_t *req,
    uint16_t parent_id
);
typedef dmxbox_rest_result_t (*dmxbox_rest_post_t)(
    httpd_req_t *req,
    uint16_t parent_id,
    cJSON *body
);
typedef dmxbox_rest_result_t (*dmxbox_rest_put_t)(
    httpd_req_t *req,
    uint16_t parent_id,
    uint16_t child_id,
    cJSON *body
);

typedef struct dmxbox_rest_container {
  const char *slug;
  bool allow_zero;
  dmxbox_rest_delete_t delete;
  dmxbox_rest_get_t get;
  dmxbox_rest_list_t list;
  dmxbox_rest_post_t post;
  dmxbox_rest_put_t put;
  const struct dmxbox_rest_child_container *first_child;
} dmxbox_rest_container_t;

typedef struct dmxbox_rest_child_container {
  const struct dmxbox_rest_child_container *next_sibling;
  const struct dmxbox_rest_container *container;
} dmxbox_rest_child_container_t;

esp_err_t dmxbox_rest_register(
    httpd_handle_t server,
    const dmxbox_rest_container_t *router
);
