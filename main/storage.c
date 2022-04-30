#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "storage";

#define CONFIG_NVS_NAMESPACE "storage"

static uint8_t first_initialization_complete = 0;
static uint8_t sta_mode_enabled = 0;

uint8_t get_first_initialization_complete(void)
{
    return first_initialization_complete;
}

void set_first_initialization_complete(uint8_t value)
{
    ESP_LOGI(TAG, "Setting 'first_init' to %d", value);

    nvs_handle_t storage_handle;
    ESP_ERROR_CHECK(nvs_open(CONFIG_NVS_NAMESPACE, NVS_READWRITE, &storage_handle));

    first_initialization_complete = value;
    ESP_ERROR_CHECK(nvs_set_u8(storage_handle, "first_init", value));

    ESP_ERROR_CHECK(nvs_commit(storage_handle));
    nvs_close(storage_handle);
}

uint8_t get_sta_mode_enabled(void)
{
    return sta_mode_enabled;
}

void set_sta_mode_enabled(uint8_t value)
{
    ESP_LOGI(TAG, "Setting 'sta_mode' to %d", value);

    nvs_handle_t storage_handle;
    ESP_ERROR_CHECK(nvs_open(CONFIG_NVS_NAMESPACE, NVS_READWRITE, &storage_handle));

    sta_mode_enabled = value;
    ESP_ERROR_CHECK(nvs_set_u8(storage_handle, "sta_mode", value));

    ESP_ERROR_CHECK(nvs_commit(storage_handle));
    nvs_close(storage_handle);
}

void handle_read_err(esp_err_t err, const char *key)
{
    switch (err)
    {
    case ESP_OK:
        break;
    case ESP_ERR_NVS_NOT_FOUND:
        ESP_LOGI(TAG, "'%s' is not set yet", key);
        break;
    default:
        ESP_LOGI(TAG, "Error reading '%s': %s", key, esp_err_to_name(err));
        break;
    }
}

void storage_init(void)
{
    ESP_LOGI(TAG, "Initializing storage");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGI(TAG, "nvs_flash_init returned %s, erasing storage", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    nvs_handle_t storage_handle;
    ESP_ERROR_CHECK(nvs_open(CONFIG_NVS_NAMESPACE, NVS_READWRITE, &storage_handle));

    err = nvs_get_u8(storage_handle, "first_init", &first_initialization_complete);
    handle_read_err(err, "first_init");

    err = nvs_get_u8(storage_handle, "sta_mode", &sta_mode_enabled);
    handle_read_err(err, "sta_mode");

    nvs_close(storage_handle);
}

void storage_factory_reset(void)
{
    ESP_LOGI(TAG, "Erasing storage");
    ESP_ERROR_CHECK(nvs_flash_erase());
}
