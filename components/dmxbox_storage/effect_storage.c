#include "effect_storage.h"
#include "dmxbox_storage.h"
#include "entry.h"
#include "esp_err.h"
#include "private.h"
#include <esp_check.h>
#include <nvs.h>

static const char EFFECTS_NS[] = "dmxbox/effect";
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

static size_t effect_size(size_t step_count) {
  return sizeof(dmxbox_effect_t) + (step_count - 1) * sizeof(uint16_t);
}

dmxbox_effect_t *dmxbox_effect_alloc(size_t step_count) {
  size_t size = effect_size(step_count);
  dmxbox_effect_t *effect = calloc(1, size);
  if (effect) {
    effect->step_count = step_count;
  }
  return effect;
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
      dmxbox_storage_get_blob(EFFECTS_NS, key, &size, result ? &buffer : NULL),
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
    dmxbox_storage_entry_t *page
) {
  return dmxbox_storage_list_blobs(EFFECTS_NS, NULL, NULL, skip, count, page);
}

esp_err_t dmxbox_effect_create(const dmxbox_effect_t *effect, uint16_t *id) {
  if (!id) {
    return ESP_ERR_INVALID_ARG;
  }

  nvs_handle_t storage;
  ESP_RETURN_ON_ERROR(
      nvs_open(EFFECTS_NS, NVS_READWRITE, &storage),
      TAG,
      "failed to open %s",
      EFFECTS_NS
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
