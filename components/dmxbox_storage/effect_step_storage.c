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

esp_err_t dmxbox_storage_get_effect_step(
    uint16_t effect_id,
    uint16_t step_id,
    size_t *size,
    dmxbox_storage_effect_step_t **result
) {
  if (!size || !result) {
    return ESP_ERR_INVALID_ARG;
  }

  *size = 0;
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

  ESP_GOTO_ON_ERROR(
      dmxbox_storage_get_blob_malloc(storage, key, size, buffer),
      close_storage,
      TAG,
      "failed to get the blob for key '%s'",
      key
  );

  *result = buffer;

close_storage:
  nvs_close(storage);
  return ret;
}

esp_err_t dmxbox_storage_put_effect_step(
    uint16_t effect_id,
    uint16_t step_id,
    size_t size,
    const dmxbox_storage_effect_step_t *value
) {
  esp_err_t ret = ESP_OK;

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
