#include "effect_storage.h"
#include "dmxbox_storage.h"
#include "entry.h"
#include "esp_err.h"
#include "private.h"
#include <esp_check.h>
#include <nvs.h>

static const char EFFECTS_NS[] = "dmxbox/effect";
static const char TAG[] = "dmxbox_storage_effect";

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
  size_t size = 0;
  void *buffer = NULL;
  ESP_RETURN_ON_ERROR(
      dmxbox_storage_get_blob(
          EFFECTS_NS,
          0,
          effect_id,
          &size,
          result ? &buffer : NULL
      ),
      TAG,
      "failed to get the blob for effect id '%u'",
      effect_id
  );
  if (result) {
    *result = buffer;
  } else {
    free(buffer);
  }
  return ESP_OK;
}

esp_err_t dmxbox_effect_delete(uint16_t effect_id) {
  ESP_RETURN_ON_ERROR(
      dmxbox_storage_delete_blob(EFFECTS_NS, 0, effect_id),
      TAG,
      "failed to delete the blob for effect id '%u'",
      effect_id
  );
  return ESP_OK;
}

esp_err_t dmxbox_effect_list(
    uint16_t skip,
    uint16_t *count,
    dmxbox_storage_entry_t *page
) {
  return dmxbox_storage_list_blobs(EFFECTS_NS, 0, skip, count, page);
}

esp_err_t dmxbox_effect_create(const dmxbox_effect_t *effect, uint16_t *id) {
  return dmxbox_storage_create_blob(
      EFFECTS_NS,
      0,
      effect,
      effect_size(effect->step_count),
      id
  );
}
