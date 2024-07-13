#include "serializer.h"
#include "cJSON.h"
#include <esp_log.h>
#include <stdlib.h>

static const char TAG[] = "dmxbox_serializer";

typedef cJSON *(*dmxbox_to_json_func_t)(const void *object);
typedef bool (*dmxbox_from_json_func_t)(const cJSON *json, void *object);

#define TRAILING_COUNT_OFFSET 0

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
    ESP_LOGE(
        TAG,
        "deserializing [+%zu] '%s' not found",
        entry->offset,
        entry->json_name
    );
  }
  return item;
}

static bool get_intvalue(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    int *result
) {
  const cJSON *item = get_item(entry, json);
  if (!item) {
    return false;
  }
  if (!cJSON_IsNumber(item)) {
    ESP_LOGE(
        TAG,
        "deserializing [+%zu] '%s' => not a number",
        entry->offset,
        entry->json_name
    );
    return false;
  }
  *result = item->valueint;
  return true;
}

static bool get_uintvalue(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    uint32_t *result
) {
  int intvalue;
  if (!get_intvalue(entry, json, &intvalue)) {
    return false;
  }
  if (intvalue < 0) {
    ESP_LOGE(
        TAG,
        "deserializing [+%zu] '%s' is negative: %d",
        entry->offset,
        entry->json_name,
        intvalue
    );
    return false;
  }
  *result = (uint32_t)intvalue;
  return true;
}

cJSON *dmxbox_serialize_u8(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  const uint8_t *ptr = (uint8_t *)at_offset(object, entry->offset);
  ESP_LOGI(
      TAG,
      "serializing [+%zu] '%s' => %" PRIu8 " (u8)",
      entry->offset,
      entry->json_name,
      *ptr
  );
  return cJSON_CreateNumber(*ptr);
}

bool dmxbox_deserialize_u8(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    void *object
) {
  uint32_t uintvalue;
  if (!get_uintvalue(entry, json, &uintvalue)) {
    return false;
  }
  if (uintvalue > UINT8_MAX) {
    ESP_LOGE(
        TAG,
        "deserializing [+%zu] '%s' too large for u8: %" PRIu32,
        entry->offset,
        entry->json_name,
        uintvalue
    );
    return false;
  }
  ESP_LOGI(
      TAG,
      "deserializing [+%zu] '%s' => %" PRIu32 " (u8)",
      entry->offset,
      entry->json_name,
      uintvalue
  );
  uint8_t *ptr = (uint8_t *)at_offset(object, entry->offset);
  *ptr = (uint8_t)uintvalue;
  return true;
}

cJSON *dmxbox_serialize_u16(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  const uint16_t *ptr = (uint16_t *)at_offset(object, entry->offset);
  ESP_LOGI(
      TAG,
      "serializing [+%zu] '%s' => %" PRIu16 " (u16)",
      entry->offset,
      entry->json_name,
      *ptr
  );
  return cJSON_CreateNumber(*ptr);
}

bool dmxbox_deserialize_u16(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    void *object
) {
  uint32_t uintvalue;
  if (!get_uintvalue(entry, json, &uintvalue)) {
    return false;
  }
  if (uintvalue > UINT16_MAX) {
    ESP_LOGE(
        TAG,
        "deserializing [+%zu] '%s' too large for u16: %" PRIu32,
        entry->offset,
        entry->json_name,
        uintvalue
    );
    return false;
  }
  ESP_LOGI(
      TAG,
      "deserializing [+%zu] '%s' => %" PRIu32 " (u16)",
      entry->offset,
      entry->json_name,
      uintvalue
  );
  uint16_t *ptr = (uint16_t *)at_offset(object, entry->offset);
  *ptr = (uint16_t)uintvalue;
  return true;
}

cJSON *dmxbox_serialize_u32(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  const uint32_t *ptr = (uint32_t *)at_offset(object, entry->offset);
  ESP_LOGI(
      TAG,
      "serializing [+%zu] '%s' => %" PRIu32 " (u32)",
      entry->offset,
      entry->json_name,
      *ptr
  );
  return cJSON_CreateNumber(*ptr);
}

bool dmxbox_deserialize_u32(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    void *object
) {
  uint32_t uintvalue;
  if (!get_uintvalue(entry, json, &uintvalue)) {
    return false;
  }
  ESP_LOGI(
      TAG,
      "deserializing [+%zu] '%s' => %" PRIu32 " (u32)",
      entry->offset,
      entry->json_name,
      uintvalue
  );
  uint32_t *ptr = (uint32_t *)at_offset(object, entry->offset);
  *ptr = uintvalue;
  return true;
}

cJSON *dmxbox_serialize_item(
    const dmxbox_serializer_entry_t *entry,
    const void *object
) {
  ESP_LOGI(TAG, "serializing [+%zu] item", entry->offset);
  const void *ptr = at_offset(object, entry->offset);
  dmxbox_to_json_func_t func = entry->context[0];
  return func(ptr);
}

bool dmxbox_deserialize_item(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    void *object
) {
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
  ESP_LOGI(TAG, "serializing %zu trailing item(s)", *count_ptr);

  dmxbox_to_json_func_t ptr_to_json = entry->context[1];

  cJSON *json = cJSON_CreateArray();
  if (!json) {
    return NULL;
  }
  for (size_t i = 0; i < *count_ptr; i++) {
    ESP_LOGI(TAG, "serializing trailing item %zu", i);
    cJSON *item = ptr_to_json(array_ptr);
    if (!item) {
      goto error;
    }
    if (!cJSON_AddItemToArray(json, item)) {
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
  // dmxbox_deserialize knows about trailing arrays and will already have
  // allocated the correct size and filled in the count field
  const cJSON *json_array = get_item(entry, json);
  if (!json_array) {
    return false;
  }

  dmxbox_from_json_func_t from_json = entry->context[2];
  size_t json_array_size = cJSON_GetArraySize(json_array);

  void *array = at_offset(object, entry->offset);
  size_t array_item_size = get_trailing_array_item_size(entry);

  for (size_t i = 0; i < json_array_size; i++) {
    cJSON *item = cJSON_GetArrayItem(array, i);
    if (!item) {
      ESP_LOGE(TAG, "couldn't get array index %zu", i);
      return false;
    }
    if (!from_json(item, array)) {
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

void *dmxbox_deserialize_object(
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

  while (entry && entry->json_name) {
    if (!entry->deserialize(entry, json, object)) {
      goto error;
    }
    entry++;
  }
  return object;

error:
  free(object);
  return NULL;
}
