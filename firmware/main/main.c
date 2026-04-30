// =============================================================================
// XIAO ESP32-S3 Sense — real-time audio spectrum analyzer.
//
// Pipeline (one task per stage would be possible, but at 16 kHz with a 1024
// FFT we have ~64 ms of headroom per hop, which is comfortably handled in a
// single task. Keeping it linear also keeps the on-wire ordering trivial):
//
//   PDM mic -> I2S DMA -> int16 block -> Hann window -> radix-2 FFT
//           -> dB magnitudes -> binary frame -> WebSocket broadcast.
//
// Everything outside the analysis task (Wi-Fi, esp_http_server) runs on the
// IDF protocol task and event loop, so this file mostly does setup and then
// runs a tight processing loop.
// =============================================================================

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "audio_capture.h"
#include "dsp_pipeline.h"
#include "web_server.h"
#include "wifi_sta.h"

static const char *TAG = "main";

#define FFT_SIZE  CONFIG_AUDIO_FFT_SIZE

static int16_t *s_audio_block;
static uint8_t *s_frame_buf;

static void analysis_task(void *arg)
{
    (void)arg;

    while (1) {
        esp_err_t err = audio_capture_read(s_audio_block, FFT_SIZE, portMAX_DELAY);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "audio read failed: %s", esp_err_to_name(err));
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        size_t out_len = 0;
        err = dsp_pipeline_process(s_audio_block,
                                   s_frame_buf, DSP_FRAME_BYTES,
                                   &out_len);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "dsp failed: %s", esp_err_to_name(err));
            continue;
        }

        web_server_broadcast(s_frame_buf, out_len);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "XIAO Audio Spectrum Analyzer — booting");

    s_audio_block = heap_caps_malloc(FFT_SIZE * sizeof(int16_t),
                                     MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    s_frame_buf   = heap_caps_malloc(DSP_FRAME_BYTES,
                                     MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    if (!s_audio_block || !s_frame_buf) {
        ESP_LOGE(TAG, "Out of memory allocating analysis buffers");
        return;
    }

    ESP_ERROR_CHECK(audio_capture_init());
    ESP_ERROR_CHECK(dsp_pipeline_init());

    if (wifi_sta_start_blocking() != ESP_OK) {
        ESP_LOGE(TAG, "Wi-Fi failed to associate; the analyzer will still run "
                      "but no clients can connect. Reconfigure SSID/password "
                      "in `idf.py menuconfig` -> XIAO Audio Analyzer.");
    } else {
        ESP_ERROR_CHECK(web_server_start());
    }

    // The processing loop runs on its own task so the IDF main task can exit
    // cleanly. Stack of 6 KB is comfortable: the FFT scratch lives on the
    // heap, this task just walks pointers.
    BaseType_t ok = xTaskCreatePinnedToCore(analysis_task,
                                            "analysis",
                                            6144, NULL,
                                            5, NULL,
                                            APP_CPU_NUM);
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "Failed to create analysis task");
    }
}
