#include <esp_log.h>
#include <nvs_flash.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "dmxbox_const.h"
#include "private.h"

static const char *TAG = "storage";

static const char artnet_snapshots_ns[] = "dmxbox/snaps";

static const char *key_first_run_completed = "first_init";
static const char *key_sta_mode_enabled = "sta_mode";
static const char *key_hostname = "hostname";
static const char *key_native_universe = "native_uni";
static const char *key_effect_control_universe = "effect_uni";

static const char *default_hostname = "dmx-box";
static const uint16_t default_native_universe = 0;
static const uint16_t default_effect_control_universe = 1;

static uint8_t first_run_completed_ = 0;
static uint8_t sta_mode_enabled_ = 0;
static char hostname_[16] = "dmx-box";
static uint16_t native_universe_ = 0;
static uint16_t effect_control_universe_ = 0;

uint8_t dmxbox_get_first_run_completed() { return first_run_completed_; }
uint8_t dmxbox_get_sta_mode_enabled() { return sta_mode_enabled_; }
const char *dmxbox_get_hostname() { return hostname_; }
const char *dmxbox_get_default_hostname() { return default_hostname; }
uint16_t dmxbox_get_native_universe() { return native_universe_; }
uint16_t dmxbox_get_effect_control_universe() {
  return effect_control_universe_;
}

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

void dmxbox_set_native_universe(uint16_t value) {
  native_universe_ = value;
  dmxbox_storage_set_u16(key_native_universe, native_universe_);
}

void dmxbox_set_effect_control_universe(uint16_t value) {
  effect_control_universe_ = value;
  dmxbox_storage_set_u16(key_effect_control_universe, effect_control_universe_);
}

bool dmxbox_get_artnet_snapshot(
    uint16_t universe,
    uint8_t data[DMX_CHANNEL_COUNT]
) {
  size_t size = DMX_CHANNEL_COUNT;
  void *buffer;
  esp_err_t err =
      dmxbox_storage_get_blob(artnet_snapshots_ns, 0, universe, &size, &buffer);
  if (err == ESP_ERR_NOT_FOUND) {
    ESP_LOGI(TAG, "Stored snapshot for artnet universe %d not found", universe);
    return false;
  }
  ESP_ERROR_CHECK(err);

  if (size != DMX_CHANNEL_COUNT) {
    ESP_LOGE(
        TAG,
        "Stored artnet universe %d snapshot size is incorrect: %d",
        universe,
        size
    );
    return false;
  }

  memcpy(data, buffer, DMX_CHANNEL_COUNT);

  free(buffer);
  return true;
}

void dmxbox_set_artnet_snapshot(
    uint16_t universe,
    const uint8_t data[DMX_CHANNEL_COUNT]
) {
  ESP_ERROR_CHECK(dmxbox_storage_set_blob(
      artnet_snapshots_ns,
      0,
      universe,
      DMX_CHANNEL_COUNT,
      data
  ));
}

void dmxbox_storage_init() {
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
  native_universe_ = dmxbox_storage_get_u16(storage, key_native_universe);
  effect_control_universe_ =
      dmxbox_storage_get_u16(storage, key_effect_control_universe);
  nvs_close(storage);
}

void dmxbox_storage_set_defaults() {
  dmxbox_set_hostname(default_hostname);
  dmxbox_set_native_universe(default_native_universe);
  dmxbox_set_effect_control_universe(default_effect_control_universe);
}

void dmxbox_storage_factory_reset() {
  ESP_LOGI(TAG, "Erasing storage");
  ESP_ERROR_CHECK(nvs_flash_erase());
}
