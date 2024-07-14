#include <esp_dmx.h>
#include <esp_log.h>
#include <esp_spiffs.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <string.h>

#include "dmxbox_artnet.h"
#include "dmxbox_dmx_receive.h"
#include "dmxbox_dmx_send.h"
#include "dmxbox_dns.h"
#include "dmxbox_effects.h"
#include "dmxbox_espnow.h"
#include "dmxbox_httpd.h"
#include "dmxbox_led.h"
#include "dmxbox_recalc.h"
#include "dmxbox_storage.h"
#include "factory_reset.h"
#include "sdkconfig.h"
#include "webserver.h"
#include "wifi.h"

static const char *TAG = "main";

esp_err_t init_fs() {
  esp_vfs_spiffs_conf_t conf = {
      .base_path = "/www",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = false,
  };
  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    } else {
      ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    }
    return ESP_FAIL;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);
  if (ret != ESP_OK) {
    ESP_LOGE(
        TAG,
        "Failed to get SPIFFS partition information (%s)",
        esp_err_to_name(ret)
    );
  } else {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }
  return ESP_OK;
}

void app_main() {
  ESP_LOGI(TAG, "App starting...");

  dmxbox_led_start();

  dmxbox_handle_factory_reset();

  ESP_ERROR_CHECK(dmxbox_led_set(dmxbox_led_power, 1));

  dmxbox_storage_init();
  dmxbox_wifi_start();

  if (!dmxbox_get_first_run_completed()) {
    wifi_set_defaults();
    dmxbox_set_first_run_completed(1);
  }

  dmxbox_artnet_init();

  dmxbox_effects_init();

  dmxbox_espnow_init();

  ESP_ERROR_CHECK(init_fs());
  ESP_ERROR_CHECK(dmxbox_webserver_start());
  dmxbox_start_dns_server();

  xTaskCreate(dmxbox_artnet_receive_task, "ArtNet", 10000, NULL, 2, NULL);

  xTaskCreate(dmxbox_dmx_receive_task, "DMX receive", 10000, NULL, 2, NULL);

  xTaskCreate(dmxbox_effects_task, "Effect runner", 10000, NULL, 3, NULL);

  xTaskCreate(dmxbox_recalc_task, "Recalc", 10000, NULL, 4, NULL);

  xTaskCreate(dmxbox_dmx_send_task, "DMX send", 10000, NULL, 5, NULL);
}
