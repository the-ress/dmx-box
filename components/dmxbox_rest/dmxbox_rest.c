#include "dmxbox_rest.h"
#include "dmxbox_httpd.h"
#include "dmxbox_uri.h"
#include "esp_check.h"
#include "esp_http_server.h"
#include "http_parser.h"
#include "xtensa/config/specreg.h"
#include <esp_err.h>
#include <esp_log.h>
#include <stdio.h>

static const char TAG[] = "dmxbox_rest";

static esp_err_t dmxbox_rest_parse_uri(
    const dmxbox_rest_container_t *container,
    httpd_req_t *req,
    dmxbox_rest_uri_t *result
) {
  if (!result) {
    return ESP_ERR_INVALID_ARG;
  }
  result->parent_id = 0;
  result->id = 0;
  result->container = NULL;

  const char *uri = req->uri;
  uri = dmxbox_uri_match_component("api", uri);
  uri = dmxbox_uri_match_component(container->slug, uri);

  if (!uri) {
    ESP_LOGE(TAG, "uri '%s' doesn't match %s", req->uri, container->slug);
    return ESP_ERR_NOT_FOUND;
  }

  result->container = container;
  if (*uri == '\0') {
    ESP_LOGI(TAG, "uri '%s' => %s container root", req->uri, container->slug);
    return ESP_OK;
  }

  uri = dmxbox_uri_match_positive_u16(&result->id, uri);
  if (!uri) {
    ESP_LOGE(TAG, "uri %s' id not positive u16", req->uri);
    return ESP_ERR_NOT_FOUND;
  }

  if (*uri == '\0') {
    ESP_LOGI(
        TAG,
        "uri '%s' => %s id=%u",
        req->uri,
        container->slug,
        result->id
    );
    return ESP_OK;
  }

  result->parent_id = result->id;
  result->id = 0;

  for (const dmxbox_rest_child_container_t *child = container->first_child;
       child;
       child = child->next_sibling) {
    const char *child_uri =
        dmxbox_uri_match_component(child->container->slug, uri);
    if (!child_uri) {
      ESP_LOGI(
          TAG,
          "uri '%s' doesn't match %s",
          req->uri,
          child->container->slug
      );
      continue;
    }

    result->container = child->container;

    if (*child_uri == '\0') {
      ESP_LOGI(TAG, "uri '%s' matches %s", req->uri, child->container->slug);
      return ESP_OK;
    }

    child_uri = dmxbox_uri_match_positive_u16(&result->id, child_uri);
    if (!child_uri) {
      ESP_LOGE(TAG, "uri '%s' child id not positive u16", req->uri);
      return ESP_ERR_NOT_FOUND;
    }

    if (*child_uri == '\0') {
      ESP_LOGI(
          TAG,
          "uri '%s' parent_id=%u id=%u",
          req->uri,
          result->parent_id,
          result->id
      );
      return ESP_OK;
    }

    ESP_LOGE(TAG, "uri '%s' trailing segments", req->uri);
    return ESP_ERR_NOT_FOUND;
  }

  ESP_LOGE(TAG, "uri '%s' unknown child", req->uri);
  return ESP_ERR_NOT_FOUND;
}

const dmxbox_rest_result_t dmxbox_rest_200_ok = {.status = "200 OK"};
const dmxbox_rest_result_t dmxbox_rest_204_no_content = {
    .status = "204 No Content"};
const dmxbox_rest_result_t dmxbox_rest_400_bad_request = {
    .status = "400 Bad Request"};
const dmxbox_rest_result_t dmxbox_rest_404_not_found = {
    .status = "404 Not Found"};
const dmxbox_rest_result_t dmxbox_rest_405_method_not_allowed = {
    .status = "405 Method Not Allowed"};
const dmxbox_rest_result_t dmxbox_rest_500_internal_server_error = {
    .status = "500 Internal Server Error"};

dmxbox_rest_result_t dmxbox_rest_result_json(cJSON *json) {
  dmxbox_rest_result_t result = dmxbox_rest_200_ok;
  result.body = json;
  return result;
}

dmxbox_rest_result_t dmxbox_rest_201_created(const char *location_format, ...) {
  dmxbox_rest_result_t result = {.status = "201 Created"};
  va_list args;
  va_start(args, location_format);
  size_t chars_needed = vsniprintf(
      result.location,
      sizeof(result.location),
      location_format,
      args
  );
  if (chars_needed >= sizeof(result.location)) {
    ESP_LOGE(TAG, "failed to create Location header");
    return dmxbox_rest_500_internal_server_error;
  }
  return result;
}

static esp_err_t
dmxbox_rest_send(httpd_req_t *req, dmxbox_rest_result_t result) {
  esp_err_t ret = ESP_OK;
  if (result.status) {
    ESP_GOTO_ON_ERROR(
        httpd_resp_set_status(req, result.status),
        exit,
        TAG,
        "failed to set HTTP status"
    );
  }
  if (result.location[0] != '\0') {
    ESP_GOTO_ON_ERROR(
        httpd_resp_set_hdr(req, "Location", result.location),
        exit,
        TAG,
        "failed to set Location"
    );
  }
  if (result.body) {
    ESP_GOTO_ON_ERROR(
        dmxbox_httpd_send_json(req, result.body),
        exit,
        TAG,
        "failed to send json response"
    );
  } else {
    ESP_GOTO_ON_ERROR(
        httpd_resp_send(req, NULL, 0),
        exit,
        TAG,
        "failed to send empty response"
    );
  }

exit:
  cJSON_free(result.body);
  return ret;
}

static esp_err_t
dmxbox_rest_get(httpd_req_t *req, const dmxbox_rest_uri_t *uri) {
  dmxbox_rest_result_t result;
  if (uri->id) {
    if (uri->container->get) {
      ESP_LOGI(
          TAG,
          "GET %s/%u (parent_id=%u)",
          uri->container->slug,
          uri->id,
          uri->parent_id
      );
      result = uri->container->get(req, uri->parent_id, uri->id);
    } else {
      ESP_LOGE(
          TAG,
          "GET %s/%u (parent_id=%u) not implemented",
          uri->container->slug,
          uri->id,
          uri->parent_id
      );
      result = dmxbox_rest_405_method_not_allowed;
    }
  } else {
    if (uri->container->list) {
      ESP_LOGI(
          TAG,
          "GET %s (list, parent_id=%u)",
          uri->container->slug,
          uri->parent_id
      );
      result = uri->container->list(req, uri->parent_id);
    } else {
      ESP_LOGE(
          TAG,
          "GET %s (list, parent_id=%u) not implemented",
          uri->container->slug,
          uri->parent_id
      );
      result = dmxbox_rest_405_method_not_allowed;
    }
  }
  return dmxbox_rest_send(req, result);
}

static esp_err_t
dmxbox_rest_post(httpd_req_t *req, const dmxbox_rest_uri_t *uri) {
  cJSON *json;
  ESP_RETURN_ON_ERROR(
      dmxbox_httpd_receive_json(req, &json),
      TAG,
      "failed to receive request json"
  );

  dmxbox_rest_result_t result;
  if (uri->id) {
    ESP_LOGE(
        TAG,
        "POST %s/%u (parent_id=%u) not allowed",
        uri->container->slug,
        uri->id,
        uri->parent_id
    );
    result = dmxbox_rest_405_method_not_allowed;
  } else {
    if (uri->container->post) {
      ESP_LOGI(
          TAG,
          "POST %s/%u (parent_id=%u)",
          uri->container->slug,
          uri->id,
          uri->parent_id
      );
      result = uri->container->post(req, uri->parent_id, json);
    } else {
      ESP_LOGE(
          TAG,
          "POST %s/%u (parent_id=%u) not implemented",
          uri->container->slug,
          uri->id,
          uri->parent_id
      );
      result = dmxbox_rest_405_method_not_allowed;
    }
  }
  cJSON_free(json);
  return dmxbox_rest_send(req, result);
}

static esp_err_t
dmxbox_rest_put(httpd_req_t *req, const dmxbox_rest_uri_t *uri) {
  cJSON *json;
  ESP_RETURN_ON_ERROR(
      dmxbox_httpd_receive_json(req, &json),
      TAG,
      "failed to receive request json"
  );

  dmxbox_rest_result_t result;
  if (uri->container->put) {
    ESP_LOGI(
        TAG,
        "PUT %s/%u (parent_id=%u)",
        uri->container->slug,
        uri->id,
        uri->parent_id
    );
    result = uri->container->put(req, uri->parent_id, uri->id, json);
  } else {
    ESP_LOGE(
        TAG,
        "PUT %s/%u (parent_id=%u) not implemented",
        uri->container->slug,
        uri->id,
        uri->parent_id
    );
    result = dmxbox_rest_405_method_not_allowed;
  }

  cJSON_free(json);
  return dmxbox_rest_send(req, result);
}

static esp_err_t dmxbox_rest_handler(httpd_req_t *req) {
  const dmxbox_rest_container_t *container = req->user_ctx;

  dmxbox_rest_uri_t uri;
  switch (dmxbox_rest_parse_uri(container, req, &uri)) {
  case ESP_OK:
    break;
  case ESP_ERR_NOT_FOUND:
    ESP_LOGE(TAG, "not found: %s", req->uri);
    return dmxbox_rest_send(req, dmxbox_rest_404_not_found);
  default:
    ESP_LOGE(TAG, "failed to parse uri: %s", req->uri);
    return dmxbox_rest_send(req, dmxbox_rest_500_internal_server_error);
  }

  switch (req->method) {
  case HTTP_GET:
    return dmxbox_rest_get(req, &uri);
  case HTTP_POST:
    return dmxbox_rest_post(req, &uri);
  case HTTP_PUT:
    return dmxbox_rest_put(req, &uri);
  default:
    return dmxbox_rest_send(req, dmxbox_rest_405_method_not_allowed);
  }
}

esp_err_t dmxbox_rest_register(
    httpd_handle_t server,
    const dmxbox_rest_container_t *router
) {
  char uri[48] = "/api/";
  if (strlcat(uri, router->slug, sizeof(uri)) >= sizeof(uri)) {
    ESP_LOGE(TAG, "slug too long: %s", router->slug);
    return ESP_ERR_NO_MEM;
  }
  if (strlcat(uri, "/*", sizeof(uri)) >= sizeof(uri)) {
    ESP_LOGE(TAG, "slug too long: %s", router->slug);
    return ESP_ERR_NO_MEM;
  }

  httpd_uri_t endpoint = {
      .uri = uri,
      .user_ctx = (void *)router,
      .method = HTTP_GET,
      .handler = dmxbox_rest_handler,
  };

  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &endpoint),
      TAG,
      "failed to register %s GET endpoint",
      router->slug
  );

  endpoint.method = HTTP_POST;
  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &endpoint),
      TAG,
      "failed to register %s POST endpoint",
      router->slug
  );

  endpoint.method = HTTP_PUT;
  ESP_RETURN_ON_ERROR(
      httpd_register_uri_handler(server, &endpoint),
      TAG,
      "failed to register %s PUT endpoint",
      router->slug
  );

  return ESP_OK;
}
