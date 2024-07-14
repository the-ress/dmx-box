#include "dmxbox_uri.h"
#include <esp_log.h>
#include <stdbool.h>
#include <stddef.h>

static const char TAG[] = "dmxbox_uri";

static const char *dmxbox_uri_consume_segment_end(const char *uri) {
  if (!uri) {
    return NULL;
  }
  if (*uri == '/') {
    uri++;
    return uri;
  }
  if (*uri == '\0') {
    return uri;
  }
  return NULL;
}

const char *dmxbox_uri_match_u16(uint16_t *result, const char *uri) {
  if (!uri || !result) {
    return NULL;
  }
  int value = 0;
  if (*uri == '/') {
    uri++;
  }
  for (; *uri >= '0' && *uri <= '9'; uri++) {
    if (value > UINT16_MAX) {
      ESP_LOGE(TAG, "u16 overflow");
      *result = 0;
      return NULL;
    }

    value *= 10;
    value += (*uri - '0');
  }
  ESP_LOGI(TAG, "after loop: '%s'", uri ? uri : "NULL");
  uri = dmxbox_uri_consume_segment_end(uri);
  ESP_LOGI(TAG, "consumed: '%s'", uri ? uri : "NULL");
  *result = uri ? value : 0;
  return uri;
}

const char *dmxbox_uri_match_positive_u16(uint16_t *result, const char *uri) {
  uri = dmxbox_uri_match_u16(result, uri);
  return uri && *result ? uri : NULL;
}

const char *dmxbox_uri_match_component(const char *expected, const char *uri) {
  if (!uri) {
    return NULL;
  }
  if (*uri == '/') {
    uri++;
  }
  for (; *uri && *expected; uri++, expected++) {
    if (*uri != *expected) {
      return NULL;
    }
  }
  if (*expected != '\0') {
    return NULL;
  }
  return dmxbox_uri_consume_segment_end(uri);
}
