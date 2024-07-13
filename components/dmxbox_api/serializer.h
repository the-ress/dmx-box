#pragma once
#include <cJSON.h>
#include <stdint.h>

struct dmxbox_serializer_entry;

typedef cJSON *(*dmxbox_ptr_to_json_func_t)(const void *object);

typedef cJSON *(*dmxbox_serialize_func_t)(
    const struct dmxbox_serializer_entry *,
    const void *object
);

typedef struct dmxbox_serializer_entry {
  size_t parent_size;
  size_t offset;
  dmxbox_serialize_func_t serialize;
  const void *context[2];
  const char *json_name;
} dmxbox_serializer_entry_t;

cJSON *
dmxbox_serialize_u8(const dmxbox_serializer_entry_t *entry, const void *object);

cJSON *dmxbox_serialize_u16(
    const dmxbox_serializer_entry_t *entry,
    const void *object
);

cJSON *dmxbox_serialize_ptr(
    const dmxbox_serializer_entry_t *entry,
    const void *object
);

cJSON *dmxbox_serialize_object(
    const dmxbox_serializer_entry_t *entry,
    const void *object
);

cJSON *dmxbox_serialize_trailing_array(
    const dmxbox_serializer_entry_t *entry,
    const void *array
);

#define BEGIN_DMXBOX_API_SERIALIZER(type, name)                                \
  static const dmxbox_serializer_entry_t name##_serializer[] = {

#define DMXBOX_API_SERIALIZE_U8(type, name)                                    \
  {.serialize = dmxbox_serialize_u8,                                           \
   .json_name = #name,                                                         \
   .parent_size = sizeof(type),                                                \
   .offset = offsetof(type, name)},

#define DMXBOX_API_SERIALIZE_U16(type, name)                                   \
  {.serialize = dmxbox_serialize_u16,                                          \
   .json_name = #name,                                                         \
   .parent_size = sizeof(type),                                                \
   .offset = offsetof(type, name)},

#define DMXBOX_API_SERIALIZE_SUB_OBJECT(type, name, to_json_fn)                \
  {.serialize = dmxbox_serialize_ptr,                                          \
   .context = {to_json_fn},                                                    \
   .json_name = #name,                                                         \
   .parent_size = sizeof(type),                                                \
   .offset = offsetof(type, name)},

#define DMXBOX_API_SERIALIZE_TRAILING_ARRAY(                                   \
    type,                                                                      \
    name,                                                                      \
    count_name,                                                                \
    to_json_fn                                                                 \
)                                                                              \
  {                                                                            \
      .json_name = #name,                                                      \
      .parent_size = sizeof(type),                                             \
      .offset = offsetof(type, name),                                          \
      .serialize = dmxbox_serialize_trailing_array,                            \
      .context =                                                               \
          {                                                                    \
              to_json_fn,                                                      \
              offsetof(type, count_name),                                      \
          },                                                                   \
  },

#define END_DMXBOX_API_SERIALIZER(type, name)                                  \
  { .json_name = NULL }                                                        \
  }                                                                            \
  ;                                                                            \
  cJSON *dmxbox_##name##_to_json(const type *object) {                         \
    return dmxbox_serialize_object(name##_serializer, object);                 \
  }
