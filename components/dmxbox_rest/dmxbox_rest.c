#include "dmxbox_rest.h"
#include "dmxbox_uri.h"
#include <esp_err.h>
#include <esp_log.h>

static const char TAG[] = "dmxbox_rest";

esp_err_t dmxbox_rest_parse_uri(
    const dmxbox_rest_container_t *container,
    httpd_req_t *req,
    dmxbox_rest_uri_t *result
) {
  if (!result) {
    return ESP_ERR_INVALID_ARG;
  }
  result->parent_id = 0;
  result->child_id = 0;
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

  uri = dmxbox_uri_match_positive_u16(&result->parent_id, uri);
  if (!uri) {
    ESP_LOGE(TAG, "uri %s' id not positive u16", req->uri);
    return ESP_ERR_NOT_FOUND;
  }

  if (*uri == '\0') {
    ESP_LOGI(
        TAG,
        "uri '%s' => %s parent_id=%u",
        req->uri,
        container->slug,
        result->parent_id
    );
    return ESP_OK;
  }

  for (const dmxbox_rest_child_container_t *child = container->first_child;
       child;
       child = child->sibling) {
    const char *child_uri =
        dmxbox_uri_match_component(child->container.slug, uri);
    if (!child_uri) {
      ESP_LOGI(
          TAG,
          "uri '%s' doesn't match %s",
          req->uri,
          child->container.slug
      );
      continue;
    }

    if (*child_uri == '\0') {
      ESP_LOGI(TAG, "uri '%s' matches %s", req->uri, child->container.slug);
      result->container = &child->container;
      result->child_id = 0;
      return ESP_OK;
    }

    child_uri = dmxbox_uri_match_positive_u16(&result->child_id, child_uri);
    if (!child_uri) {
      ESP_LOGE(TAG, "uri '%s' child id not positive u16", req->uri);
      return ESP_ERR_NOT_FOUND;
    }

    if (*child_uri == '\0') {
      ESP_LOGI(TAG, "uri '%s' child id %u", req->uri, result->child_id);
      result->container = &child->container;
      return ESP_OK;
    }

    ESP_LOGE(TAG, "uri '%s' trailing segments", req->uri);
    return ESP_ERR_NOT_FOUND;
  }

  ESP_LOGE(TAG, "uri '%s' unknown child", req->uri);
  return ESP_ERR_NOT_FOUND;
}
