#include "serializer.h"
#include "cJSON.h"
#include <esp_log.h>

static const char TAG[] = "dmxbox_serializer";

static void *at_offset(const void *ptr, size_t offset) {
  return (uint8_t *)ptr + offset;
}

cJSON *dmxbox_serialize_u8(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  ESP_LOGI(TAG, "serializing u8 at offset %u", entry->offset);
  const uint8_t *ptr = (uint8_t *)at_offset(object, entry->offset);
  return cJSON_CreateNumber(*ptr);
}

cJSON *dmxbox_serialize_u16(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  ESP_LOGI(TAG, "serializing u16 at offset %u", entry->offset);
  const uint16_t *ptr = (uint16_t *)at_offset(object, entry->offset);
  return cJSON_CreateNumber(*ptr);
}

cJSON *dmxbox_serialize_ptr(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  ESP_LOGI(TAG, "serializing ptr at offset %u", entry->offset);
  const void *ptr = at_offset(object, entry->offset);
  dmxbox_ptr_to_json_func_t func = entry->context[0];
  return func(ptr);
}

cJSON *dmxbox_serialize_object(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  cJSON *json = cJSON_CreateObject();
  if (!json) {
    return NULL;
  }
  while (entry->json_name) {
    ESP_LOGI(TAG, "serializing %s", entry->json_name);
    cJSON *item = entry->serialize(entry, object);
    if (!item) {
      goto error;
    }
    if (!cJSON_AddItemToObjectCS(json, entry->json_name, item)) {
      cJSON_free(item);
      goto error;
    }
    entry++;
  }

  return json;
error:
  cJSON_free(json);
  return NULL;
}

cJSON *dmxbox_serialize_trailing_array(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  const void *array_ptr = (size_t *)at_offset(object, entry->offset);
  // inline array is necessarily the last field in the structure,
  // so we get the item size by subtracting array offset from the parent size
  size_t item_size = entry->parent_size - entry->offset;

  dmxbox_ptr_to_json_func_t ptr_to_json = entry->context[0];
  const size_t *count_ptr =
      (size_t *)at_offset(object, (size_t)entry->context[1]);

  cJSON *json = cJSON_CreateArray();
  if (!json) {
    return NULL;
  }
  for (size_t i = 0; i < *count_ptr; i++) {
    cJSON *item = ptr_to_json(array_ptr);
    if (!item) {
      goto error;
    }
    if (!cJSON_AddItemToArray(json, item)) {
      cJSON_free(item);
      goto error;
    }

    array_ptr = (void *)((uint8_t *)array_ptr + item_size);
  }
  return json;

error:
  cJSON_free(json);
  return NULL;
}
