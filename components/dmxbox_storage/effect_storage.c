#include "effect_storage.h"
#include "private.h"
#include <esp_check.h>
#include <nvs.h>

static const char effect_ns[] = "dmxbox/effect";
static const char TAG[] = "dmxbox_storage_effect";

static esp_err_t step_key(uint16_t effect_id, char key[NVS_KEY_NAME_MAX_SIZE]) {
  if (snprintf(key, NVS_KEY_NAME_MAX_SIZE, "%x", effect_id) >=
      NVS_KEY_NAME_MAX_SIZE) {
    ESP_LOGE(TAG, "key buffer too small");
    return ESP_ERR_NO_MEM;
  }
  return ESP_OK;
}

esp_err_t dmxbox_effect_get(uint16_t effect_id, dmxbox_effect_t **result) {
  char key[NVS_KEY_NAME_MAX_SIZE];
  ESP_RETURN_ON_ERROR(
      step_key(effect_id, key),
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

