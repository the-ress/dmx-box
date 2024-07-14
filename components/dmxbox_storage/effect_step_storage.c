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

static esp_err_t step_key(
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

static size_t step_size(size_t channel_count) {
  return sizeof(dmxbox_effect_step_t) +
         (channel_count - 1) * sizeof(dmxbox_channel_level_t);
}

dmxbox_effect_step_t *
dmxbox_effect_step_alloc(size_t channel_count) {
  size_t size = step_size(channel_count);
  dmxbox_effect_step_t *effect_step = calloc(1, size);
  if (effect_step) {
    effect_step->channel_count = channel_count;
  }
  return effect_step;
}

esp_err_t dmxbox_effect_step_get(
    uint16_t effect_id,
    uint16_t step_id,
    dmxbox_effect_step_t **result
) {
  char key[NVS_KEY_NAME_MAX_SIZE];
  ESP_RETURN_ON_ERROR(
      step_key(effect_id, step_id, key),
      TAG,
      "failed to get key for effect %u step %u",
      effect_id,
      step_id
  );

  size_t size = 0;
  void *buffer = NULL;
  ESP_RETURN_ON_ERROR(
      dmxbox_storage_get_blob(
          effect_step_ns,
          key,
          &size,
          result ? &buffer : NULL
      ),
      TAG,
      "failed to get the blob for key '%s'",
      key
  );

  if (result) {
    *result = buffer;
    size_t expected_size = step_size((*result)->channel_count);
    if (expected_size != size) {
      ESP_LOGE(
          TAG,
          "effect %u step %u corrupted. blob size %u bytes, expected %u bytes "
          "because declared channel count %u. deleting all channels",
          effect_id,
          step_id,
          size,
          expected_size,
          (*result)->channel_count
      );
      (*result)->channel_count = 0;
    }
  }
  return ESP_OK;
}

esp_err_t dmxbox_effect_step_set(
    uint16_t effect_id,
    uint16_t step_id,
    const dmxbox_effect_step_t *value
) {
  esp_err_t ret = ESP_OK;
  size_t size = step_size(value->channel_count);

  nvs_handle_t storage;
  ESP_RETURN_ON_ERROR(
      nvs_open(effect_step_ns, NVS_READWRITE, &storage),
      TAG,
      "failed to open NVS"
  );

  char key[NVS_KEY_NAME_MAX_SIZE];
  ESP_GOTO_ON_ERROR(
      step_key(effect_id, step_id, key),
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
