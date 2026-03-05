#include "camera.h"

#include "esp_log.h"

static const char *TAG = "camera";

esp_err_t camera_init(void)
{
    /**
     * NOTE: The pin configuration below is a reasonable default for
     * ESP32-S3 camera modules but may need adjustment depending on
     * the exact ESP32-S3 Sense revision you have.
     *
     * If initialization fails at runtime, revisit these pin defines
     * and compare them against your board's schematic.
     */

    camera_config_t config = {
        .pin_pwdn = -1,
        .pin_reset = -1,
        .pin_xclk = 10,
        .pin_sccb_sda = 11,
        .pin_sccb_scl = 12,
        .pin_d7 = 13,
        .pin_d6 = 14,
        .pin_d5 = 15,
        .pin_d4 = 16,
        .pin_d3 = 17,
        .pin_d2 = 18,
        .pin_d1 = 19,
        .pin_d0 = 20,
        .pin_vsync = 21,
        .pin_href = 38,
        .pin_pclk = 39,
        .xclk_freq_hz = 20000000,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        .pixel_format = PIXFORMAT_RGB565,
        .frame_size = FRAMESIZE_QQVGA, // 160x120 for tinyML-friendly input
        .jpeg_quality = 12,
        .fb_count = 2,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return err;
    }

    ESP_LOGI(TAG, "Camera initialized");
    return ESP_OK;
}

camera_fb_t *camera_capture_frame(void)
{
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Failed to get camera frame buffer");
    }
    return fb;
}

