#pragma once
#include <cJSON.h>
#include <stdbool.h>
#include <stdint.h>

struct dmxbox_serializer_entry;

typedef cJSON *(*dmxbox_serialize_func_t)(
    const struct dmxbox_serializer_entry *,
    const void *object
);

typedef bool (*dmxbox_deserialize_func_t)(
    const struct dmxbox_serializer_entry *,
    const cJSON *json,
    void *object
);

typedef struct dmxbox_serializer_entry {
  size_t parent_size;
  size_t offset;
  dmxbox_serialize_func_t serialize;
  dmxbox_deserialize_func_t deserialize;
  const void *context[3];
  const char *json_name;
} dmxbox_serializer_entry_t;

cJSON *dmxbox_serialize_object(
    const dmxbox_serializer_entry_t *entry,
    const void *object
);

void *dmxbox_deserialize_object_alloc(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json
);

bool dmxbox_deserialize_object(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    void *object
);

//
// The following de/serializers are not meant to be invoked directly. They are
// called by dmxbox_serialize_object and dmxbox_deserialize_object.
//

cJSON *
dmxbox_serialize_u8(const dmxbox_serializer_entry_t *entry, const void *object);

cJSON *dmxbox_serialize_u16(
    const dmxbox_serializer_entry_t *entry,
    const void *object
);

cJSON *dmxbox_serialize_u32(
    const dmxbox_serializer_entry_t *entry,
    const void *object
);

cJSON *dmxbox_serialize_item(
    const dmxbox_serializer_entry_t *entry,
    const void *object
);

cJSON *dmxbox_serialize_trailing_array(
    const dmxbox_serializer_entry_t *entry,
    const void *array
);

bool dmxbox_deserialize_u8(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    void *object
);

bool dmxbox_deserialize_u16(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    void *object
);

bool dmxbox_deserialize_u32(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    void *object
);

bool dmxbox_deserialize_item(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    void *object
);

bool dmxbox_deserialize_trailing_array(
    const dmxbox_serializer_entry_t *entry,
    const cJSON *json,
    void *object
);

// Use the following macros to define object serializers

#define BEGIN_DMXBOX_API_SERIALIZER(type, name)                                \
  static const dmxbox_serializer_entry_t name##_serializer[] = {

#define DMXBOX_API_SERIALIZE_U8(type, name)                                    \
  {.serialize = dmxbox_serialize_u8,                                           \
   .deserialize = dmxbox_deserialize_u8,                                       \
   .json_name = #name,                                                         \
   .parent_size = sizeof(type),                                                \
   .offset = offsetof(type, name)},

#define DMXBOX_API_SERIALIZE_U16(type, name)                                   \
  {.serialize = dmxbox_serialize_u16,                                          \
   .deserialize = dmxbox_deserialize_u16,                                      \
   .json_name = #name,                                                         \
   .parent_size = sizeof(type),                                                \
   .offset = offsetof(type, name)},

#define DMXBOX_API_SERIALIZE_U32(type, name)                                   \
  {.serialize = dmxbox_serialize_u32,                                          \
   .deserialize = dmxbox_deserialize_u32,                                      \
   .json_name = #name,                                                         \
   .parent_size = sizeof(type),                                                \
   .offset = offsetof(type, name)},

// do not use _alloc deserializers!
#define DMXBOX_API_SERIALIZE_ITEM(type, name, to_json_fn, from_json_fn)        \
  {.serialize = dmxbox_serialize_item,                                         \
   .deserialize = dmxbox_deserialize_item,                                     \
   .context = {to_json_fn, from_json_fn},                                      \
   .json_name = #name,                                                         \
   .parent_size = sizeof(type),                                                \
   .offset = offsetof(type, name)},

// count field must be a size_t
#define DMXBOX_API_SERIALIZE_TRAILING_ARRAY(                                   \
    type,                                                                      \
    name,                                                                      \
    count_name,                                                                \
    to_json_fn,                                                                \
    from_json_fn                                                               \
)                                                                              \
  {                                                                            \
      .json_name = #name,                                                      \
      .parent_size = sizeof(type),                                             \
      .offset = offsetof(type, name),                                          \
      .serialize = dmxbox_serialize_trailing_array,                            \
      .deserialize = dmxbox_deserialize_trailing_array,                        \
      .context =                                                               \
          {                                                                    \
              (void *)offsetof(type, count_name),                              \
              to_json_fn,                                                      \
              from_json_fn,                                                    \
          },                                                                   \
  },

#define END_DMXBOX_API_SERIALIZER(type, name)                                  \
  { .json_name = NULL }                                                        \
  }                                                                            \
  ;                                                                            \
  cJSON *dmxbox_##name##_to_json(const type *object) {                         \
    return dmxbox_serialize_object(name##_serializer, object);                 \
  }                                                                            \
  type *dmxbox_##name##_from_json_alloc(const cJSON *json) {                   \
    return dmxbox_deserialize_object_alloc(name##_serializer, json);           \
  }                                                                            \
  bool dmxbox_##name##_from_json(const cJSON *json, type *object) {            \
    return dmxbox_deserialize_object(name##_serializer, json, object);         \
  }
