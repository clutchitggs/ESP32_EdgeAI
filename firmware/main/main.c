#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "camera.h"
#include "inference.h"
#include "alert.h"

static const char *TAG = "app_main";

// Number of consecutive drowsy frames required before raising an alert.
static const int DROWSY_FRAME_THRESHOLD = 5;

static void drowsiness_monitor_task(void *arg)
{
    (void)arg;

    int drowsy_frame_count = 0;

    while (1) {
        camera_fb_t *fb = camera_capture_frame();
        if (!fb) {
            ESP_LOGW(TAG, "Skipping frame due to capture failure");
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        /**
         * In a future revision, this is where we will:
         * - Extract a region of interest (eyes/upper face) from fb->buf.
         * - Resize and normalize the ROI to the model's expected input size.
         * - Pass the processed buffer into run_drowsiness_inference().
         *
         * For now, we simply pass the raw frame buffer pointer and use
         * the stub inference implementation.
         */

        drowsiness_state_t state = run_drowsiness_inference(
            fb->buf,
            fb->width,
            fb->height);

        esp_camera_fb_return(fb);

        if (state == STATE_DROWSY) {
            drowsy_frame_count++;
        } else {
            drowsy_frame_count = 0;
        }

        if (drowsy_frame_count >= DROWSY_FRAME_THRESHOLD) {
            alert_trigger();
        } else {
            alert_clear();
        }

        // Basic pacing; will be tuned once we profile inference time.
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "ESP32-S3 Drowsiness Detector starting up");

    if (camera_init() != ESP_OK) {
        ESP_LOGE(TAG, "Camera initialization failed; halting");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    alert_init();
    inference_init();

    xTaskCreate(
        drowsiness_monitor_task,
        "drowsiness_monitor",
        4096,
        NULL,
        5,
        NULL);
}

