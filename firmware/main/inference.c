#include "inference.h"

#include "esp_log.h"

static const char *TAG = "inference";

void inference_init(void)
{
    /**
     * Placeholder for future model initialization.
     *
     * When a trained model is available, this function will:
     * - Initialize the ML runtime (e.g., TensorFlow Lite Micro interpreter).
     * - Allocate and configure the tensor arena.
     * - Prepare input/output tensors.
     */
    ESP_LOGI(TAG, "Inference engine initialized (stub)");
}

drowsiness_state_t run_drowsiness_inference(uint8_t *frame_data, int width, int height)
{
    (void)frame_data;
    (void)width;
    (void)height;

    /**
     * Stub implementation.
     *
     * For now, always report STATE_AWAKE so that the rest of the
     * firmware can be developed and tested without a real model.
     *
     * Once the tinyML model is integrated, this function will:
     * - Preprocess the input as needed (normalize, reshape).
     * - Invoke the model.
     * - Interpret the model's output logits/probabilities and map
     *   them to STATE_AWAKE or STATE_DROWSY.
     */

    return STATE_AWAKE;
}

