#pragma once
#include <esp_err.h>
#include <esp_wifi.h>
#include <stdint.h>

typedef struct dmxbox_wifi_scan_result {
  uint16_t count;
  wifi_ap_record_t records[1];
} dmxbox_wifi_scan_result_t;

typedef void (*dmxbox_wifi_scan_callback_t)(dmxbox_wifi_scan_result_t *);

void dmxbox_wifi_scan_on_done();

esp_err_t dmxbox_wifi_scan_register_callback(dmxbox_wifi_scan_callback_t);
esp_err_t dmxbox_wifi_scan_start();
