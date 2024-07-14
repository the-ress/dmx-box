#include "serializer.h"
#include "cJSON.h"
#include <esp_log.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static const char TAG[] = "dmxbox_serializer";

typedef cJSON *(*dmxbox_to_json_func_t)(const void *object);
typedef bool (*dmxbox_from_json_func_t)(const cJSON *json, void *object);

#define TRAILING_COUNT_OFFSET 0
#define DESERIALIZE_LOG(level, x, format, ...)                                 \
  ESP_LOG_LEVEL_LOCAL(                                                         \
      level,                                                                   \
      TAG,                                                                     \
      "deserializing '%s' [+%zu] " format,                                     \
      x->json_name,                                                            \
      x->offset __VA_OPT__(, ) __VA_ARGS__                                     \
  );
#define DESERIALIZE_LOGE(...) DESERIALIZE_LOG(ESP_LOG_ERROR, __VA_ARGS__)
#define DESERIALIZE_LOGI(...) DESERIALIZE_LOG(ESP_LOG_INFO, __VA_ARGS__)

#define SERIALIZE_LOG(level, x, format, ...)                                   \
  ESP_LOG_LEVEL_LOCAL(                                                         \
      level,                                                                   \
      TAG,                                                                     \
      "serializing '%s' [+%zu] " format,                                       \
      x->json_name,                                                            \
      x->offset __VA_OPT__(, ) __VA_ARGS__                                     \
  );
#define SERIALIZE_LOGE(...) SERIALIZE_LOG(ESP_LOG_ERROR, __VA_ARGS__)
#define SERIALIZE_LOGI(...) SERIALIZE_LOG(ESP_LOG_INFO, __VA_ARGS__)

static void *at_offset(const void *ptr, size_t offset) {
  return (uint8_t *)ptr + offset;
}

static size_t
get_trailing_array_item_size(const dmxbox_serializer_entry_t *trailing_array) {
  // trailing array is necessarily the last field in the structure,
  // so we get the item size by subtracting array offset from the parent size
  return trailing_array->parent_size - trailing_array->offset;
}

static const dmxbox_serializer_entry_t *
find_trailing_array(const dmxbox_serializer_entry_t *entry) {
  while (entry && entry->json_name) {
    if (entry->deserialize == dmxbox_deserialize_trailing_array) {
      return entry;
    }
    entry++;
  }
  return NULL;
}

static size_t get_trailing_array_count(
    const dmxbox_serializer_entry_t *trailing_array,
    const cJSON *json
) {
  const cJSON *json_array =
      cJSON_GetObjectItemCaseSensitive(json, trailing_array->json_name);
  return json_array ? cJSON_GetArraySize(json_array) : 0;
}

static const cJSON *
get_item(const dmxbox_serializer_entry_t *entry, const cJSON *json) {
  const cJSON *item = cJSON_GetObjectItemCaseSensitive(json, entry->json_name);
  if (!item) {
    DESERIALIZE_LOGE(entry, " not found")
    ESP_LOGE(
        TAG,
        "deserializing [+%zu] '%s' not found",
        entry->offset,
        entry->json_name
    );
  }
  return item;
}

static const cJSON *
validate_number(const cJSON *item, double min, double max, const char *type) {
  if (!cJSON_IsNumber(item)) {
    ESP_LOGE(TAG, "not a number");
    return NULL;
  }
  if (item->valuedouble < min) {
    ESP_LOGE(TAG, "too small for %s", type);
    return NULL;
  }
  if (item->valuedouble > max) {
    ESP_LOGE(TAG, "too large for %s", type);
    return NULL;
  }
  return item;
}

static const cJSON *validate_u8(const cJSON *item) {
  return validate_number(item, 0, UINT8_MAX, "u8");
}

static const cJSON *validate_u16(const cJSON *item) {
  return validate_number(item, 0, UINT16_MAX, "u16");
}

static const cJSON *validate_u32(const cJSON *item) {
  return validate_number(item, 0, UINT32_MAX, "u32");
}

bool dmxbox_u8_from_json(const cJSON *json, uint8_t *value) {
  if (validate_u8(json)) {
    *value = (uint8_t)json->valueint;
    return true;
  }
  return false;
}

bool dmxbox_u16_from_json(const cJSON *json, uint16_t *value) {
  if (validate_u16(json)) {
    *value = (uint16_t)json->valueint;
    return true;
  }
  return false;
}

bool dmxbox_u32_from_json(const cJSON *json, uint32_t *value) {
  if (validate_u32(json)) {
    *value = (uint32_t)json->valuedouble;
    return true;
  }
  return false;
}

cJSON *dmxbox_u8_to_json(uint8_t value) { return cJSON_CreateNumber(value); }
cJSON *dmxbox_u16_to_json(uint16_t value) { return cJSON_CreateNumber(value); }
cJSON *dmxbox_u32_to_json(uint32_t value) { return cJSON_CreateNumber(value); }

cJSON *dmxbox_serialize_u8(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  const uint8_t *ptr = (uint8_t *)at_offset(object, entry->offset);
  SERIALIZE_LOGI(entry, "(u8) %" PRIu8, *ptr);
  return dmxbox_u8_to_json(*ptr);
}

cJSON *dmxbox_serialize_u16(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  const uint16_t *ptr = (uint16_t *)at_offset(object, entry->offset);
  SERIALIZE_LOGI(entry, "(u16) %" PRIu16, *ptr);
  return cJSON_CreateNumber(*ptr);
}

cJSON *dmxbox_serialize_u32(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  const uint32_t *ptr = (uint32_t *)at_offset(object, entry->offset);
  SERIALIZE_LOGI(entry, "(u32) %" PRIu32, *ptr);
  return cJSON_CreateNumber(*ptr);
}

bool dmxbox_deserialize_u8(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    void *object
) {
  const cJSON *item = get_item(entry, json);
  if (!item) {
    return false;
  }
  DESERIALIZE_LOGI(entry, "(u8)");
  uint8_t *ptr = (uint8_t *)at_offset(object, entry->offset);
  return dmxbox_u8_from_json(item, ptr);
}

bool dmxbox_deserialize_u16(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    void *object
) {
  const cJSON *item = get_item(entry, json);
  if (!item) {
    return false;
  }
  DESERIALIZE_LOGI(entry, "(u16)");
  uint16_t *ptr = (uint16_t *)at_offset(object, entry->offset);
  return dmxbox_u16_from_json(item, ptr);
}

bool dmxbox_deserialize_u32(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    void *object
) {
  const cJSON *item = get_item(entry, json);
  if (!item) {
    return false;
  }
  DESERIALIZE_LOGI(entry, "(u32)");
  uint32_t *ptr = (uint32_t *)at_offset(object, entry->offset);
  return dmxbox_u32_from_json(item, ptr);
}

cJSON *dmxbox_serialize_item(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  SERIALIZE_LOGI(entry, "item");
  const void *ptr = at_offset(object, entry->offset);
  dmxbox_to_json_func_t func = entry->context[0];
  return func(ptr);
}

bool dmxbox_deserialize_item(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    void *object
) {
  DESERIALIZE_LOGI(entry, "item");
  const cJSON *item = get_item(entry, json);
  if (!item) {
    return false;
  }
  void *ptr = at_offset(object, entry->offset);
  dmxbox_from_json_func_t func = entry->context[1];
  return func(item, ptr);
}

cJSON *dmxbox_serialize_trailing_array(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  const void *array_ptr = (size_t *)at_offset(object, entry->offset);
  size_t item_size = get_trailing_array_item_size(entry);

  const size_t *count_ptr = (size_t *)
      at_offset(object, (size_t)entry->context[TRAILING_COUNT_OFFSET]);
  SERIALIZE_LOGI(
      entry,
      "trailing array of %zu %zu-byte item(s)",
      *count_ptr,
      item_size
  );

  dmxbox_to_json_func_t ptr_to_json = entry->context[1];

  cJSON *json = cJSON_CreateArray();
  if (!json) {
    return NULL;
  }
  for (size_t i = 0; i < *count_ptr; i++) {
    SERIALIZE_LOGI(entry, "item %zu", i);
    cJSON *item = ptr_to_json(array_ptr);
    if (!item) {
      SERIALIZE_LOGE(entry, "item %zu failed to serialize", i);
      goto error;
    }
    if (!cJSON_AddItemToArray(json, item)) {
      DESERIALIZE_LOGE(entry, "failed to add item %zu to json", i);
      cJSON_free(item);
      goto error;
    }

    array_ptr = at_offset(array_ptr, item_size);
  }
  return json;

error:
  cJSON_free(json);
  return NULL;
}

bool dmxbox_deserialize_trailing_array(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    void *object
) {
  // dmxbox_deserialize_object knows about trailing arrays and will already
  // have allocated the correct size and filled in the count field
  const cJSON *json_array = get_item(entry, json);
  if (!json_array) {
    return false;
  }

  dmxbox_from_json_func_t from_json = entry->context[2];
  size_t json_array_count = cJSON_GetArraySize(json_array);
  DESERIALIZE_LOGI(entry, "array of %zu item(s)", json_array_count);

  void *array = at_offset(object, entry->offset);
  size_t array_item_size = get_trailing_array_item_size(entry);

  for (size_t i = 0; i < json_array_count; i++) {
    DESERIALIZE_LOGI(entry, "item %zu", i);
    cJSON *json_item = cJSON_GetArrayItem(json_array, i);
    if (!json_item) {
      DESERIALIZE_LOGE(entry, "failed to get item %zu", i);
      return false;
    }
    if (!from_json(json_item, array)) {
      DESERIALIZE_LOGE(entry, "failed to deserialize item %zu", i);
      return false;
    }
    array = at_offset(array, array_item_size);
  }
  return true;
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
    cJSON *item = entry->serialize(entry, object);
    if (!item) {
      DESERIALIZE_LOGE(entry, "failed to serialize");
      goto error;
    }
    if (!cJSON_AddItemToObjectCS(json, entry->json_name, item)) {
      DESERIALIZE_LOGE(entry, "failed to add to json");
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

static bool dmxbox_deserialize_object_impl(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    void *object
) {
  while (entry && entry->json_name) {
    if (!entry->deserialize(entry, json, object)) {
      DESERIALIZE_LOGE(entry, "failed to deserialize");
      return false;
    }
    entry++;
  }
  return true;
}

bool dmxbox_deserialize_object(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    void *object
) {
  const dmxbox_serializer_entry_t *trailing_array = find_trailing_array(entry);
  if (trailing_array) {
    ESP_LOGE(TAG, "trailing arrays only supported in _alloc deserializers");
    return false;
  }
  return dmxbox_deserialize_object_impl(entry, json, object);
}

void *dmxbox_deserialize_object_alloc(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json
) {
  size_t static_size = entry->parent_size;
  size_t trailing_count = 0;
  size_t trailing_item_size = 0;

  const dmxbox_serializer_entry_t *trailing_array = find_trailing_array(entry);
  if (trailing_array) {
    trailing_item_size = get_trailing_array_item_size(trailing_array);
    trailing_count = get_trailing_array_count(trailing_array, json);
    static_size -= trailing_item_size;
    ESP_LOGI(
        TAG,
        "static size = %zu, trailing array item size = %zu, count = %zu",
        static_size,
        trailing_item_size,
        trailing_count
    );
  } else {
    ESP_LOGI(TAG, "no trailing array, static size = %zu", static_size);
  }

  void *object = calloc(1, static_size + (trailing_item_size * trailing_count));
  if (!object) {
    return NULL;
  }

  if (trailing_array) {
    size_t *count_ptr = at_offset(
        object,
        (size_t)trailing_array->context[TRAILING_COUNT_OFFSET]
    );
    *count_ptr = trailing_count;
  }

  if (dmxbox_deserialize_object_impl(entry, json, object)) {
    return object;
  }

  free(object);
  return NULL;
}
