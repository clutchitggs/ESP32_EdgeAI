#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// On-the-wire frame layout
// -----------------------------------------------------------------------------
// Each WebSocket frame starts with the fixed-size header below, followed by
//   - DSP_WAVE_LEN  int16_t  wave samples  (time-domain, decimated for plotting)
//   - DSP_SPEC_BINS int8_t   spectrum bins (dB FS, clamped to [-128, 0])
//
// Total payload size is DSP_FRAME_BYTES. The browser parses with a DataView.
// =============================================================================

#define DSP_MAGIC      0x41554441u  // 'AUDA' little-endian
#define DSP_WAVE_LEN   256
#define DSP_SPEC_BINS  256

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;
    uint32_t seq;
    float    peak_hz;
    float    peak_db;
    uint16_t sample_rate;
    uint16_t n_wave;
    uint16_t n_spec;
    uint16_t reserved;
} dsp_frame_header_t;
#pragma pack(pop)

#define DSP_FRAME_BYTES (sizeof(dsp_frame_header_t)        \
                         + DSP_WAVE_LEN * sizeof(int16_t)  \
                         + DSP_SPEC_BINS * sizeof(int8_t))

/**
 * @brief Initialise FFT tables, window coefficients and scratch buffers.
 *
 * Must be called once before dsp_pipeline_process().
 */
esp_err_t dsp_pipeline_init(void);

/**
 * @brief Run one analysis frame.
 *
 * @param audio        Pointer to CONFIG_AUDIO_FFT_SIZE int16 samples (mono).
 * @param out_buf      Destination buffer; must be at least DSP_FRAME_BYTES.
 * @param out_buf_size Size of @p out_buf.
 * @param out_len      Receives the number of bytes written (DSP_FRAME_BYTES).
 *
 * @return ESP_OK on success.
 */
esp_err_t dsp_pipeline_process(const int16_t *audio,
                               uint8_t       *out_buf,
                               size_t         out_buf_size,
                               size_t        *out_len);

#ifdef __cplusplus
}
#endif
