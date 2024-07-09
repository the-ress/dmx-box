#include "esp_dmx.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <string.h>

#include "artnet/artnet.h"
#include "const.h"
#include "dmx/dmx_receive.h"
#include "dmx/dmx_send.h"
#include "dns.h"
#include "effects/effects.h"
#include "factory_reset.h"
#include "dmxbox_led.h"
#include "recalc.h"
#include "sdkconfig.h"
#include "storage.h"
#include "dmxbox_httpd.h"
#include "wifi.h"

static const char *TAG = "main";

#define CONFIG_RECALC_PERIOD 100

esp_err_t init_fs(void) {
  esp_vfs_spiffs_conf_t conf = {
      .base_path = "/www",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = false
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
  if (ret != ESP_OK){
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

void app_main(void) {
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

  dmxbox_artnet_initialize();

  dmxbox_effects_initialize();

  ESP_ERROR_CHECK(init_fs());
  ESP_ERROR_CHECK(dmxbox_webserver_start());
  dmxbox_start_dns_server();

  xTaskCreate(dmxbox_artnet_receive_task, "ArtNet", 10000, NULL, 2, NULL);

  xTaskCreate(dmxbox_dmx_receive_task, "DMX receive", 10000, NULL, 2, NULL);

  xTaskCreate(dmxbox_effects_task, "Effect runner", 10000, NULL, 3, NULL);

  xTaskCreate(dmxbox_dmx_send_task, "DMX send", 10000, NULL, 4, NULL);

  uint8_t data[DMX_PACKET_SIZE_MAX];
  while (1) {
    bool artnet_active;
    bool dmx_out_active;

    dmxbox_recalc(data, &artnet_active, &dmx_out_active);

    taskENTER_CRITICAL(&dmxbox_dmx_out_spinlock);
    memcpy(dmxbox_dmx_out_data, data, DMX_PACKET_SIZE_MAX);
    taskEXIT_CRITICAL(&dmxbox_dmx_out_spinlock);

    dmxbox_set_artnet_active(artnet_active);
    dmxbox_set_dmx_out_active(dmx_out_active);

    vTaskDelay(CONFIG_RECALC_PERIOD / portTICK_PERIOD_MS);
  }
}
