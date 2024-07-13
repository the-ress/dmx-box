#pragma once
#include <nvs_flash.h>

nvs_handle_t dmxbox_storage_open(nvs_open_mode_t open_mode);
bool dmxbox_storage_check_error(const char *key, esp_err_t err);

void dmxbox_storage_set_u8(const char *key, uint8_t value);
uint8_t dmxbox_storage_get_u8(nvs_handle_t storage, const char *key);

void dmxbox_storage_set_str(const char *key, const char *value);
bool dmxbox_storage_get_str(
    nvs_handle_t storage,
    const char *key,
    char *buffer,
    size_t buffer_size
);

esp_err_t dmxbox_storage_set_blob(
    nvs_handle_t storage,
    const char *key,
    size_t size,
    const void *value
);

// caller must free() the *buffer when ESP_OK
esp_err_t dmxbox_storage_get_blob_from_storage(
    nvs_handle_t storage,
    const char *key,
    size_t *size,
    void **buffer
);
// caller must free() the *buffer when ESP_OK
esp_err_t dmxbox_storage_get_blob(
    const char *ns,
    const char *key,
    size_t *size,
    void **buffer
);

esp_err_t
dmxbox_storage_get_blob_size(const char *ns, const char *key, size_t *size);
