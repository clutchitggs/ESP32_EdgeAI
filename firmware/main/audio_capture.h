#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the on-board PDM microphone.
 *
 * The XIAO ESP32-S3 Sense routes its digital MEMS microphone to:
 *   - PDM clock : GPIO42
 *   - PDM data  : GPIO41
 *
 * This function configures I2S0 as a PDM-RX channel at the sample rate
 * requested via Kconfig (CONFIG_AUDIO_SAMPLE_RATE_HZ) and enables the
 * channel so that reads start succeeding immediately afterwards.
 *
 * @return ESP_OK on success, otherwise an esp_err_t propagated from the
 *         I2S driver.
 */
esp_err_t audio_capture_init(void);

/**
 * @brief Read a contiguous block of int16 PCM samples from the microphone.
 *
 * Blocks until @p n_samples have been read or @p timeout ticks elapse.
 * The PDM->PCM decimation is handled internally by the I2S peripheral, so
 * the caller receives ready-to-process signed 16-bit mono samples.
 *
 * @param dst         Destination buffer, must hold at least n_samples elements.
 * @param n_samples   Number of int16 samples to read.
 * @param timeout     Maximum time to wait for the full block.
 *
 * @return ESP_OK on a complete read, ESP_ERR_TIMEOUT on a short read,
 *         or another esp_err_t from the I2S driver.
 */
esp_err_t audio_capture_read(int16_t *dst, size_t n_samples, TickType_t timeout);

/**
 * @brief Configured sample rate, in Hertz.
 */
uint32_t audio_capture_sample_rate(void);

#ifdef __cplusplus
}
#endif
