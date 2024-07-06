#include "esp_log.h"
#include "nvs_flash.h"
#include <stdint.h>

static const char *TAG = "storage";

static const char *dmxbox_nvs_ns = "dmxbox";

static uint8_t first_run_completed_ = 0;
static uint8_t sta_mode_enabled_ = 0;

static const char *key_first_run_completed = "first_init";
static const char *key_sta_mode_enabled = "sta_mode";

static nvs_handle_t dmxbox_storage_open(nvs_open_mode_t open_mode) {
  nvs_handle_t storage;
  ESP_ERROR_CHECK(nvs_open(dmxbox_nvs_ns, open_mode, &storage));
  return storage;
}

static void dmxbox_storage_set(const char *key, uint8_t value) {
  ESP_LOGI(TAG, "Setting '%s' to %d", key, value);
  nvs_handle_t storage = dmxbox_storage_open(NVS_READWRITE);
  ESP_ERROR_CHECK(nvs_set_u8(storage, key, value));
  ESP_ERROR_CHECK(nvs_commit(storage));
  nvs_close(storage);
}

static uint8_t dmxbox_storage_get(nvs_handle_t storage, const char *key) {
  uint8_t result;
  esp_err_t err = nvs_get_u8(storage, key, &result);
  switch (err) {
  case ESP_OK:
    return result;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGI(TAG, "'%s' found", key);
    return 0;
  default:
    ESP_LOGI(TAG, "Error reading '%s': %s", key, esp_err_to_name(err));
    return 0;
  }
}

uint8_t dmxbox_get_first_run_completed(void) { return first_run_completed_; }
uint8_t dmxbox_get_sta_mode_enabled(void) { return sta_mode_enabled_; }

void dmxbox_set_first_run_completed(uint8_t value) {
  dmxbox_storage_set(key_first_run_completed, value);
}

void dmxbox_set_sta_mode_enabled(uint8_t value) {
  dmxbox_storage_set(key_sta_mode_enabled, value);
}

void dmxbox_storage_init(void) {
  ESP_LOGI(TAG, "Initializing storage");

  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_LOGI(
        TAG,
        "nvs_flash_init returned %s, erasing storage",
        esp_err_to_name(err)
    );
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);

  nvs_handle_t storage = dmxbox_storage_open(NVS_READONLY);
  first_run_completed_ = dmxbox_storage_get(storage, key_first_run_completed);
  sta_mode_enabled_ = dmxbox_storage_get(storage, key_sta_mode_enabled);
  nvs_close(storage);
}

void storage_factory_reset(void) {
  ESP_LOGI(TAG, "Erasing storage");
  ESP_ERROR_CHECK(nvs_flash_erase());
}
