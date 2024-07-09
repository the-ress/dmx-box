#pragma once
#include <esp_err.h>
#include "sdkconfig.h"

#ifndef CONFIG_DMXBOX_WEBSEVER_MAX_SOCKETS
#define CONFIG_DMXBOX_WEBSEVER_MAX_SOCKETS 10
#endif

esp_err_t dmxbox_webserver_start(void);

