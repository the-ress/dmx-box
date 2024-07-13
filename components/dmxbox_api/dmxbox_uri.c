#include "dmxbox_uri.h"
#include <esp_log.h>
#include <stdbool.h>
#include <stddef.h>

static const char TAG[] = "dmxbox_uri";

static bool dmxbox_uri_is_segment_end(const char *str) {
  return *str == '/' || *str == '\0';
}

const char *dmxbox_uri_match_u16(uint16_t *result, const char *uri) {
  if (!uri) {
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
  if (dmxbox_uri_is_segment_end(uri)) {
    *result = value;
    return uri;
  }
  *result = 0;
  return NULL;
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
  if (dmxbox_uri_is_segment_end(uri)) {
    return *expected == '\0' ? uri : NULL;
  }
  return NULL;
}

