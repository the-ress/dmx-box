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

static size_t step_size(size_t channel_count) {
  return sizeof(dmxbox_effect_step_t) +
         (channel_count - 1) * sizeof(dmxbox_channel_level_t);
}

dmxbox_effect_step_t *dmxbox_effect_step_alloc(size_t channel_count) {
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
  size_t size = 0;
  void *buffer = NULL;
  ESP_RETURN_ON_ERROR(
      dmxbox_storage_get_blob(
          effect_step_ns,
          effect_id,
          step_id,
          &size,
          result ? &buffer : NULL
      ),
      TAG,
      "failed to get the blob for effect %u step %u",
      effect_id,
      step_id
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
  size_t size = step_size(value->channel_count);
  return dmxbox_storage_set_blob(
      effect_step_ns,
      effect_id,
      step_id,
      size,
      value
  );
}
