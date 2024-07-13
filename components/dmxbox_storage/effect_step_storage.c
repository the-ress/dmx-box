#include "effect_step_storage.h"
#include "dmxbox_storage.h"
#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "private.h"
#include <esp_check.h>
#include <stdio.h>
#include <stdlib.h>

static const char TAG[] = "dmxbox_storage_effect_step";
static const char effect_step_ns[] = "dmxbox/steps";

static esp_err_t dmxbox_storage_effect_step_key(
    uint16_t effect_id,
    uint16_t step_id,
    char key[NVS_KEY_NAME_MAX_SIZE]
) {
  if (snprintf(key, NVS_KEY_NAME_MAX_SIZE, "%x:%x", effect_id, step_id) >=
      NVS_KEY_NAME_MAX_SIZE) {
    ESP_LOGE(TAG, "key buffer too small");
    return ESP_ERR_NO_MEM;
  }
  return ESP_OK;
}

static size_t dmxbox_storage_effect_step_size(size_t channel_count) {
  return sizeof(dmxbox_storage_effect_step_t) +
         (channel_count - 1) * sizeof(dmxbox_storage_channel_level_t);
}

static size_t dmxbox_storage_effect_step_channel_count(size_t blob_size) {
  static const size_t header_size = sizeof(dmxbox_storage_effect_step_t) -
                                    sizeof(dmxbox_storage_channel_level_t);
  if (blob_size < header_size) {
    return 0;
  }
  return (blob_size - header_size) / sizeof(dmxbox_storage_channel_level_t);
}

dmxbox_storage_effect_step_t *
dmxbox_storage_effect_step_alloc(size_t channel_count) {
  size_t size = dmxbox_storage_effect_step_size(channel_count);
  dmxbox_storage_effect_step_t *effect_step = calloc(1, size);
  if (effect_step) {
    effect_step->channel_count = channel_count;
  }
  return effect_step;
}

esp_err_t dmxbox_storage_effect_step_get(
    uint16_t effect_id,
    uint16_t step_id,
    dmxbox_storage_effect_step_t **result
) {
  if (!result) {
    return ESP_ERR_INVALID_ARG;
  }

  *result = NULL;

  char key[NVS_KEY_NAME_MAX_SIZE];
  ESP_RETURN_ON_ERROR(
      dmxbox_storage_effect_step_key(effect_id, step_id, key),
      TAG,
      "failed to get key for effect %u step %u",
      effect_id,
      step_id
  );

  nvs_handle_t storage;
  ESP_RETURN_ON_ERROR(
      nvs_open(effect_step_ns, NVS_READONLY, &storage),
      TAG,
      "failed to open NVS"
  );

  esp_err_t ret = ESP_OK;
  void *buffer = NULL;

  size_t size;
  ESP_GOTO_ON_ERROR(
      dmxbox_storage_get_blob_malloc(storage, key, &size, &buffer),
      close_storage,
      TAG,
      "failed to get the blob for key '%s'",
      key
  );

  *result = buffer;
  size_t channel_count_from_size =
      dmxbox_storage_effect_step_channel_count(size);
  if (channel_count_from_size != (*result)->channel_count) {
    ESP_LOGE(
        TAG,
        "effect %u step %u corrupted. blob size %u bytes = %u channels, "
        "declared channel count %u. deleting all channels",
        effect_id,
        step_id,
        size,
        channel_count_from_size,
        (*result)->channel_count
    );
    (*result)->channel_count = 0;
  }

close_storage:
  nvs_close(storage);
  if (ret == ESP_ERR_NVS_NOT_FOUND) {
    ret = ESP_ERR_NOT_FOUND;
  }
  return ret;
}

esp_err_t dmxbox_storage_effect_step_set(
    uint16_t effect_id,
    uint16_t step_id,
    const dmxbox_storage_effect_step_t *value
) {
  esp_err_t ret = ESP_OK;
  size_t size = dmxbox_storage_effect_step_size(value->channel_count);

  nvs_handle_t storage;
  ESP_RETURN_ON_ERROR(
      nvs_open(effect_step_ns, NVS_READWRITE, &storage),
      TAG,
      "failed to open NVS"
  );

  char key[NVS_KEY_NAME_MAX_SIZE];
  ESP_GOTO_ON_ERROR(
      dmxbox_storage_effect_step_key(effect_id, step_id, key),
      close_storage,
      TAG,
      "failed to get key for effect %u step %u",
      effect_id,
      step_id
  );

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
