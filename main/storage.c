#include "esp_log.h"
#include "nvs_flash.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static const char *TAG = "storage";

static const char *dmxbox_nvs_ns = "storage";
static const char *key_first_run_completed = "first_init";
static const char *key_sta_mode_enabled = "sta_mode";
static const char *key_hostname = "hostname";

static uint8_t first_run_completed_ = 0;
static uint8_t sta_mode_enabled_ = 0;
static char hostname_[16] = "dmx-box";

static nvs_handle_t dmxbox_storage_open(nvs_open_mode_t open_mode) {
  nvs_handle_t storage;
  ESP_ERROR_CHECK(nvs_open(dmxbox_nvs_ns, open_mode, &storage));
  return storage;
}

static bool dmxbox_storage_check_error(const char *key, esp_err_t err) {
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

static void dmxbox_storage_set_u8(const char *key, uint8_t value) {
  ESP_LOGI(TAG, "Setting '%s' to %d", key, value);
  nvs_handle_t storage = dmxbox_storage_open(NVS_READWRITE);
  ESP_ERROR_CHECK(nvs_set_u8(storage, key, value));
  ESP_ERROR_CHECK(nvs_commit(storage));
  nvs_close(storage);
}

static void dmxbox_storage_set_str(const char *key, const char *value) {
  ESP_LOGI(TAG, "Setting '%s' to '%s'", key, value);
  nvs_handle_t storage = dmxbox_storage_open(NVS_READWRITE);
  ESP_ERROR_CHECK(nvs_set_str(storage, key, value));
  ESP_ERROR_CHECK(nvs_commit(storage));
  nvs_close(storage);
}

static uint8_t dmxbox_storage_get_u8(nvs_handle_t storage, const char *key) {
  ESP_LOGI(TAG, "Reading '%s' u8", key);
  uint8_t result;
  esp_err_t err = nvs_get_u8(storage, key, &result);
  if (dmxbox_storage_check_error(key, err)) {
    return result;
  }
  return 0;
}

static bool dmxbox_storage_get_str(
    nvs_handle_t storage,
    const char *key,
    char *buffer,
    size_t buffer_size
) {
  ESP_LOGI(TAG, "Reading '%s' str", key);
  esp_err_t err = nvs_get_str(storage, key, buffer, &buffer_size);
  return dmxbox_storage_check_error(key, err);
}

uint8_t dmxbox_get_first_run_completed(void) { return first_run_completed_; }
uint8_t dmxbox_get_sta_mode_enabled(void) { return sta_mode_enabled_; }
const char *dmxbox_get_hostname(void) { return hostname_; }

void dmxbox_set_first_run_completed(uint8_t value) {
  first_run_completed_ = value;
  dmxbox_storage_set_u8(key_first_run_completed, value);
}

void dmxbox_set_sta_mode_enabled(uint8_t value) {
  sta_mode_enabled_ = value;
  dmxbox_storage_set_u8(key_sta_mode_enabled, value);
}

bool dmxbox_set_hostname(const char *value) {
  if (strlcpy(hostname_, value, sizeof(hostname_)) >= sizeof(hostname_)) {
    return false;
  }
  dmxbox_storage_set_str(key_hostname, hostname_);
  return true;
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
  first_run_completed_ =
      dmxbox_storage_get_u8(storage, key_first_run_completed);
  sta_mode_enabled_ = dmxbox_storage_get_u8(storage, key_sta_mode_enabled);
  dmxbox_storage_get_str(storage, key_hostname, hostname_, sizeof(hostname_));
  nvs_close(storage);
}

void dmxbox_storage_factory_reset(void) {
  ESP_LOGI(TAG, "Erasing storage");
  ESP_ERROR_CHECK(nvs_flash_erase());
}
