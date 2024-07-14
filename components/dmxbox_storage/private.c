#include "private.h"
#include <esp_check.h>
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>

// TODO change to dmxbox
#define DMXBOX_NVS_NS "storage"

static const char *TAG = "dmx_box_storage";

nvs_handle_t dmxbox_storage_open(nvs_open_mode_t open_mode) {
  nvs_handle_t storage;
  esp_err_t result = nvs_open(DMXBOX_NVS_NS, open_mode, &storage);
  if (result == ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGW(
        TAG,
        "namespace " DMXBOX_NVS_NS " doesn't exist, trying to create"
    );

    ESP_ERROR_CHECK(nvs_open(DMXBOX_NVS_NS, NVS_READWRITE, &storage));
    nvs_close(storage);

    result = nvs_open(DMXBOX_NVS_NS, open_mode, &storage);
  }
  ESP_ERROR_CHECK(result);
  return storage;
}

bool dmxbox_storage_check_error(const char *key, esp_err_t err) {
  switch (err) {
  case ESP_OK:
    return true;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGI(TAG, "'%s' not found", key);
    return false;
  default:
    ESP_LOGI(TAG, "Error reading '%s': %s", key, esp_err_to_name(err));
    return false;
  }
}

void dmxbox_storage_set_u8(const char *key, uint8_t value) {
  ESP_LOGI(TAG, "Setting '%s' to %d", key, value);
  nvs_handle_t storage = dmxbox_storage_open(NVS_READWRITE);
  ESP_ERROR_CHECK(nvs_set_u8(storage, key, value));
  ESP_ERROR_CHECK(nvs_commit(storage));
  nvs_close(storage);
}

void dmxbox_storage_set_str(const char *key, const char *value) {
  ESP_LOGI(TAG, "Setting '%s' to '%s'", key, value);
  nvs_handle_t storage = dmxbox_storage_open(NVS_READWRITE);
  ESP_ERROR_CHECK(nvs_set_str(storage, key, value));
  ESP_ERROR_CHECK(nvs_commit(storage));
  nvs_close(storage);
}

uint8_t dmxbox_storage_get_u8(nvs_handle_t storage, const char *key) {
  ESP_LOGI(TAG, "Reading '%s' u8", key);
  uint8_t result;
  esp_err_t err = nvs_get_u8(storage, key, &result);
  if (dmxbox_storage_check_error(key, err)) {
    return result;
  }
  return 0;
}

bool dmxbox_storage_get_str(
    nvs_handle_t storage,
    const char *key,
    char *buffer,
    size_t buffer_size
) {
  ESP_LOGI(TAG, "Reading '%s' str", key);
  esp_err_t err = nvs_get_str(storage, key, buffer, &buffer_size);
  return dmxbox_storage_check_error(key, err);
}

esp_err_t dmxbox_storage_get_blob_from_storage(
    nvs_handle_t storage,
    const char *key,
    size_t *size,
    void **result
) {
  if (!size) {
    return ESP_ERR_INVALID_ARG;
  }

  ESP_RETURN_ON_ERROR(
      nvs_get_blob(storage, key, NULL, size),
      TAG,
      "failed to get size of blob '%s'",
      key
  );

  if (!result) {
    return ESP_OK;
  }

  esp_err_t ret = ESP_OK;
  void *buffer = malloc(*size);
  if (!buffer) {
    ESP_LOGE(TAG, "failed to allocate %u-byte buffer", *size);
    return ESP_ERR_NO_MEM;
  }

  ESP_GOTO_ON_ERROR(
      nvs_get_blob(storage, key, buffer, size),
      free_buffer,
      TAG,
      "failed to read blob '%s'",
      key
  );

  *result = buffer;
  return ESP_OK;

free_buffer:
  if (buffer) {
    free(buffer);
  }

  return ret;
}

esp_err_t dmxbox_storage_get_blob(
    const char *ns,
    const char *key,
    size_t *size,
    void **buffer
) {
  nvs_handle_t storage;
  esp_err_t ret = nvs_open(ns, NVS_READONLY, &storage);
  switch (ret) {
  case ESP_OK:
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    return ESP_ERR_NOT_FOUND;
  default:
    return ret;
  }
  ret = dmxbox_storage_get_blob_from_storage(storage, key, size, buffer);
  nvs_close(storage);
  if (ret == ESP_ERR_NVS_NOT_FOUND) {
    return ESP_ERR_NOT_FOUND;
  }
  return ret;
}

static uint16_t default_id_parser(const char *key, void *context) {
  char *last = NULL;
  long value = strtol(key, &last, 16);
  if (*last != '\0' || value < 0) {
    return 0;
  }
  return (uint16_t)value;
}

esp_err_t dmxbox_storage_list_blobs(
    const char *ns,
    dmxbox_storage_parse_id_t id_parser,
    void *id_parser_ctx,
    uint16_t skip,
    uint16_t *count,
    dmxbox_storage_entry_t *page
) {
  if (!count || !page) {
    return ESP_ERR_INVALID_ARG;
  }
  if (!id_parser) {
    id_parser = default_id_parser;
  }

  nvs_handle_t storage;
  esp_err_t ret = nvs_open(ns, NVS_READONLY, &storage);
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

      page[read].id = id_parser(entry_info.key, id_parser_ctx);
      if (!page[read].id) {
        ESP_LOGI(TAG, "key '%s' doesn't match", entry_info.key);
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
      page[read].data = buffer;
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
