#include "esp_log.h"
#include "private.h"
#include <nvs_flash.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static const char *TAG = "storage";

static const char *key_first_run_completed = "first_init";
static const char *key_sta_mode_enabled = "sta_mode";
static const char *key_hostname = "hostname";

static uint8_t first_run_completed_ = 0;
static uint8_t sta_mode_enabled_ = 0;
static char hostname_[16] = "dmx-box";

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
