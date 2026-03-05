#pragma once

#include "esp_err.h"
#include "esp_camera.h"

/**
 * @brief Initialize the camera on the ESP32-S3 Sense board.
 *
 * This uses a typical configuration for the ESP32-S3 Sense module.
 * If your hardware revision differs, adjust the pinout in camera.c.
 */
esp_err_t camera_init(void);

/**
 * @brief Capture a single frame from the camera.
 *
 * The returned frame buffer is owned by the camera driver and must be
 * returned with esp_camera_fb_return() when no longer needed.
 */
camera_fb_t *camera_capture_frame(void);

