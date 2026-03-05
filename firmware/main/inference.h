#pragma once

#include <stdint.h>

typedef enum
{
    STATE_AWAKE = 0,
    STATE_DROWSY = 1
} drowsiness_state_t;

/**
 * @brief Initialize inference engine resources.
 *
 * In the initial skeleton this is a stub. When the real model is
 * integrated, this function will set up the ML runtime (e.g. TFLM).
 */
void inference_init(void);

/**
 * @brief Run drowsiness inference on a preprocessed frame.
 *
 * @param frame_data Pointer to pixel data (e.g., grayscale) in row-major order.
 * @param width Width of the frame in pixels.
 * @param height Height of the frame in pixels.
 * @return drowsiness_state_t Inferred state (awake/drowsy).
 */
drowsiness_state_t run_drowsiness_inference(uint8_t *frame_data, int width, int height);

