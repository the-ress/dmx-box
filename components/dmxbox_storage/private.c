#include "private.h"
#include <esp_check.h>
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <stdio.h>
#include <string.h>

// TODO change to dmxbox
#define DMXBOX_NVS_NS "storage"

static const char *TAG = "dmx_box_storage";

static bool make_next_id_key(char *key, size_t key_size, uint16_t parent_id) {
  if (parent_id) {
    return snprintf(key, key_size, "%x:next_id", parent_id) < key_size;
  }
  return strlcpy(key, "next_id", key_size) < key_size;
}

static bool
make_blob_key(char *key, size_t key_size, uint16_t parent_id, uint16_t id) {
  if (parent_id && id) {
    return snprintf(key, key_size, "%x:%x", parent_id, id) < key_size;
  }
  return snprintf(key, key_size, "%x", id) < key_size;
}

static bool parse_blob_key(const char *key, uint16_t *parent_id, uint16_t *id) {
  ESP_LOGI(TAG, "parsing blob key '%s'", key);
  char *last = NULL;
  long value = strtol(key, &last, 16);
  if (value < 0) {
    return false;
  }
  if (*last == '\0') {
    *parent_id = 0;
    *id = value;
    return true;
  }
  if (*last == ':') {
    *parent_id = value;
    value = strtol(last + 1, &last, 16);
    if (value < 0 || *last != '\0') {
      return false;
    }
    *id = value;
    return true;
  }
  return false;
}

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

void dmxbox_storage_set_u16(const char *key, uint16_t value) {
  ESP_LOGI(TAG, "Setting '%s' to %d", key, value);
  nvs_handle_t storage = dmxbox_storage_open(NVS_READWRITE);
  ESP_ERROR_CHECK(nvs_set_u16(storage, key, value));
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

uint16_t dmxbox_storage_get_u16(nvs_handle_t storage, const char *key) {
  ESP_LOGI(TAG, "Reading '%s' u16", key);
  uint16_t result;
  esp_err_t err = nvs_get_u16(storage, key, &result);
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
    uint16_t parent_id,
    uint16_t id,
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
  char key[NVS_KEY_NAME_MAX_SIZE];
  if (make_blob_key(key, sizeof(key), parent_id, id)) {
    ret = dmxbox_storage_get_blob_from_storage(storage, key, size, buffer);
  } else {
    ret = ESP_ERR_NO_MEM;
  }
  nvs_close(storage);
  if (ret == ESP_ERR_NVS_NOT_FOUND) {
    return ESP_ERR_NOT_FOUND;
  }
  return ret;
}

esp_err_t
dmxbox_storage_delete_blob_from_storage(nvs_handle_t storage, const char *key) {
  ESP_RETURN_ON_ERROR(
      nvs_erase_key(storage, key),
      TAG,
      "failed to delete blob '%s'",
      key
  );
  ESP_RETURN_ON_ERROR(nvs_commit(storage), TAG, "failed to commit");

  return ESP_OK;
}

esp_err_t
dmxbox_storage_delete_blob(const char *ns, uint16_t parent_id, uint16_t id) {
  nvs_handle_t storage;
  esp_err_t ret = nvs_open(ns, NVS_READWRITE, &storage);
  switch (ret) {
  case ESP_OK:
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    return ESP_ERR_NOT_FOUND;
  default:
    return ret;
  }
  char key[NVS_KEY_NAME_MAX_SIZE];
  if (make_blob_key(key, sizeof(key), parent_id, id)) {
    ret = dmxbox_storage_delete_blob_from_storage(storage, key);
  } else {
    ret = ESP_ERR_NO_MEM;
  }
  nvs_close(storage);
  if (ret == ESP_ERR_NVS_NOT_FOUND) {
    return ESP_ERR_NOT_FOUND;
  }
  return ret;
}

esp_err_t dmxbox_storage_list_blobs(
    const char *ns,
    uint16_t parent_id,
    uint16_t skip,
    uint16_t *count,
    dmxbox_storage_entry_t *page
) {
  if (!count || !page) {
    return ESP_ERR_INVALID_ARG;
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

      uint16_t entry_parent_id;
      if (!parse_blob_key(entry_info.key, &entry_parent_id, &page[read].id)) {
        ESP_LOGE(TAG, "key '%s' failed to parse", entry_info.key);
        goto next;
      }

      ESP_LOGI(
          TAG,
          "key '%s' id=%u parent_id=%u ",
          entry_info.key,
          page[read].id,
          entry_parent_id
      );
      if (entry_parent_id != parent_id) {
        ESP_LOGI(TAG, "parent_id doesn't match %u", parent_id);
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

esp_err_t dmxbox_storage_create_blob(
    const char *ns,
    uint16_t parent_id,
    const void *data,
    size_t size,
    uint16_t *id
) {
  if (!ns || !id) {
    return ESP_ERR_INVALID_ARG;
  }

  esp_err_t ret = ESP_OK;
  nvs_handle_t storage;
  ESP_RETURN_ON_ERROR(
      nvs_open(ns, NVS_READWRITE, &storage),
      TAG,
      "failed to open %s",
      ns
  );

  char key[NVS_KEY_NAME_MAX_SIZE];
  if (!make_next_id_key(key, sizeof(key), parent_id)) {
    ESP_LOGE(TAG, "failed to create next_id key for parent_id '%u'", parent_id);
    ret = ESP_ERR_NO_MEM;
    goto exit;
  }

  ret = nvs_get_u16(storage, key, id);
  switch (ret) {
  case ESP_OK:
    ESP_LOGI(TAG, "%s = %u", key, *id);
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
      nvs_set_u16(storage, key, *id + 1),
      exit,
      TAG,
      "failed to save next_id"
  );

  if (!make_blob_key(key, sizeof(key), parent_id, *id)) {
    ESP_LOGE(
        TAG,
        "failed to create key for parent_id %u, id %u",
        parent_id,
        *id
    );
    ret = ESP_ERR_NO_MEM;
    goto exit;
  }

  ESP_GOTO_ON_ERROR(
      nvs_set_blob(storage, key, data, size),
      exit,
      TAG,
      "failed to save %u-byte blob",
      size
  );

  ESP_GOTO_ON_ERROR(nvs_commit(storage), exit, TAG, "failed to commit");

exit:
  nvs_close(storage);
  return ret;
}

esp_err_t dmxbox_storage_set_blob(
    const char *ns,
    uint16_t parent_id,
    uint16_t id,
    size_t size,
    const void *value
) {
  esp_err_t ret = ESP_OK;

  nvs_handle_t storage;
  ESP_RETURN_ON_ERROR(
      nvs_open(ns, NVS_READWRITE, &storage),
      TAG,
      "failed to open NVS"
  );

  char key[NVS_KEY_NAME_MAX_SIZE];
  if (!make_blob_key(key, sizeof(key), parent_id, id)) {
    ESP_LOGE(TAG, "failed to make key for parent_id %u, id %u", parent_id, id);
    ret = ESP_ERR_NO_MEM;
    goto close_storage;
  }

  ESP_GOTO_ON_ERROR(
      nvs_set_blob(storage, key, value, size),
      close_storage,
      TAG,
      "failed to write blob '%s'",
      key
  );

  ESP_GOTO_ON_ERROR(
      nvs_commit(storage),
      close_storage,
      TAG,
      "failed to commit NVS"
  );

close_storage:
  nvs_close(storage);
  return ret;
}
