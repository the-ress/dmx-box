#include "effect_storage.h"
#include "dmxbox_storage.h"
#include "esp_err.h"
#include "private.h"
#include <esp_check.h>
#include <nvs.h>

static const char effect_ns[] = "dmxbox/effect";
static const char TAG[] = "dmxbox_storage_effect";
static const char NEXT_ID[] = "next_id";

static esp_err_t make_key(uint16_t effect_id, char key[NVS_KEY_NAME_MAX_SIZE]) {
  if (snprintf(key, NVS_KEY_NAME_MAX_SIZE, "%x", effect_id) >=
      NVS_KEY_NAME_MAX_SIZE) {
    ESP_LOGE(TAG, "key buffer too small");
    return ESP_ERR_NO_MEM;
  }
  return ESP_OK;
}

static uint16_t parse_key(char key[NVS_KEY_NAME_MAX_SIZE]) {
  char *last = NULL;
  long value = strtol(key, &last, 16);
  if (*last != '\0' || value < 0) {
    return 0;
  }
  return (uint16_t)value;
}

esp_err_t dmxbox_effect_get(uint16_t effect_id, dmxbox_effect_t **result) {
  char key[NVS_KEY_NAME_MAX_SIZE];
  ESP_RETURN_ON_ERROR(
      make_key(effect_id, key),
      TAG,
      "failed to get key for effect %u",
      effect_id
  );

  size_t size = 0;
  void *buffer = NULL;
  ESP_RETURN_ON_ERROR(
      dmxbox_storage_get_blob(effect_ns, key, &size, result ? &buffer : NULL),
      TAG,
      "failed to get the blob for key '%s'",
      key
  );

  if (result) {
    *result = buffer;
  } else {
    free(buffer);
  }
  return ESP_OK;
}

esp_err_t dmxbox_effect_list(
    uint16_t skip,
    uint16_t *count,
    dmxbox_effect_entry_t *page
) {
  if (!count || !page) {
    return ESP_ERR_INVALID_ARG;
  }

  nvs_handle_t storage;
  esp_err_t ret = nvs_open(effect_ns, NVS_READONLY, &storage);
  switch (ret) {
  case ESP_OK:
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    *count = 0;
    return ESP_OK;
  default:
    ESP_LOGE(TAG, "failed to open storage");
    return ret;
  }

  nvs_iterator_t iterator = NULL;
  ESP_GOTO_ON_ERROR(
      nvs_entry_find_in_handle(storage, NVS_TYPE_BLOB, &iterator),
      exit,
      TAG,
      "failed to find first"
  );

  size_t read = 0;
  while (ret == ESP_OK && read < *count) {
    if (skip == 0) {
      nvs_entry_info_t entry_info;
      nvs_entry_info(iterator, &entry_info);

      page[read].id = parse_key(entry_info.key);
      if (!page[read].id) {
        ESP_LOGE(TAG, "skipping invalid key '%s'", entry_info.key);
        goto next;
      }

      void *buffer = NULL;
      ret = dmxbox_storage_get_blob_from_storage(
          storage,
          entry_info.key,
          &page[read].size,
          &buffer
      );
      if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to get blob '%s'", entry_info.key);
        goto next;
      }
      page[read].effect = buffer;
      read++;
    } else {
      skip--;
    }

  next:
    ret = nvs_entry_next(&iterator);
  }
  if (ret == ESP_ERR_NVS_NOT_FOUND) {
    ret = ESP_OK;
  }
  *count = read;

exit:
  if (iterator) {
    nvs_release_iterator(iterator);
  }
  nvs_close(storage);
  return ret;
}

esp_err_t dmxbox_effect_create(const dmxbox_effect_t *effect, uint16_t *id) {
  if (!id) {
    return ESP_ERR_INVALID_ARG;
  }

  nvs_handle_t storage;
  ESP_RETURN_ON_ERROR(
      nvs_open(effect_ns, NVS_READWRITE, &storage),
      TAG,
      "failed to open %s",
      effect_ns
  );

  esp_err_t ret = nvs_get_u16(storage, NEXT_ID, id);
  switch (ret) {
  case ESP_OK:
    ESP_LOGI(TAG, "next_id = %u", *id);
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGI(TAG, "no next_id, starting from 1");
    ret = ESP_OK;
    *id = 1;
    break;
  default:
    goto exit;
  }

  ESP_GOTO_ON_ERROR(
      nvs_set_u16(storage, NEXT_ID, *id + 1),
      exit,
      TAG,
      "failed to save next_id"
  );

  char key[NVS_KEY_NAME_MAX_SIZE];
  ESP_GOTO_ON_ERROR(make_key(*id, key), exit, TAG, "failed to make key");

  ESP_GOTO_ON_ERROR(
      nvs_set_blob(storage, key, effect, sizeof(*effect)),
      exit,
      TAG,
      "failed to save effect"
  );

  ESP_GOTO_ON_ERROR(nvs_commit(storage), exit, TAG, "failed to commit");

exit:
  nvs_close(storage);
  return ret;
}
