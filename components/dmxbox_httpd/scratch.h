#pragma once

#ifndef CONFIG_DMXBOX_HTTPD_SCRATCH_SIZE
#define CONFIG_DMXBOX_HTTPD_SCRATCH_SIZE 10240
#endif

// only access this from the esp_http_server task
extern char dmxbox_httpd_scratch[CONFIG_DMXBOX_HTTPD_SCRATCH_SIZE];
