#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Connect to the Wi-Fi network configured via Kconfig.
 *
 * Initialises NVS, the TCP/IP stack, the default event loop, the Wi-Fi
 * driver in station mode, and blocks until association either succeeds or
 * the retry budget is exhausted.
 *
 * On success the device's IP address is logged at INFO level.
 *
 * @return ESP_OK if associated and got an IP, ESP_FAIL if the retry budget
 *         was exhausted, or another esp_err_t from the underlying drivers.
 */
esp_err_t wifi_sta_start_blocking(void);

#ifdef __cplusplus
}
#endif
