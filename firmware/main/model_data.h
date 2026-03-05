#pragma once

#include <stdint.h>

/**
 * @brief Model data for the eye-state classifier.
 *
 * In a future stage, the TensorFlow Lite model produced by the
 * training script will be converted into a C array and stored in
 * model_data.c, with these extern symbols providing access.
 */
extern const unsigned char g_eye_state_model_data[];
extern const int g_eye_state_model_data_len;

