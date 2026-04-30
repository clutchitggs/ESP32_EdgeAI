#include "dsp_pipeline.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "esp_dsp.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "sdkconfig.h"

#include "audio_capture.h"

static const char *TAG = "dsp_pipeline";

#define FFT_SIZE       (CONFIG_AUDIO_FFT_SIZE)
#define FFT_HALF       (FFT_SIZE / 2)

// -----------------------------------------------------------------------------
// Pre-allocated scratch (PSRAM-friendly: ~24 KB at FFT_SIZE=1024).
//   - s_window   : Hann window coefficients
//   - s_complex  : interleaved complex working buffer for the in-place radix-2
//   - s_mag      : magnitude (linear) per positive bin
// -----------------------------------------------------------------------------
static float   *s_window;
static float   *s_complex;
static float   *s_mag;
static uint32_t s_seq;

esp_err_t dsp_pipeline_init(void)
{
    s_window  = heap_caps_malloc(FFT_SIZE * sizeof(float),     MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    s_complex = heap_caps_malloc(2 * FFT_SIZE * sizeof(float), MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    s_mag     = heap_caps_malloc(FFT_HALF * sizeof(float),     MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);

    if (!s_window || !s_complex || !s_mag) {
        ESP_LOGE(TAG, "Out of memory allocating DSP buffers");
        return ESP_ERR_NO_MEM;
    }

    // Pre-compute a Hann window once. esp-dsp writes N coefficients in-place.
    dsps_wind_hann_f32(s_window, FFT_SIZE);

    esp_err_t err = dsps_fft2r_init_fc32(NULL, FFT_SIZE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "dsps_fft2r_init_fc32 failed: %s", esp_err_to_name(err));
        return err;
    }

    s_seq = 0;
    ESP_LOGI(TAG, "FFT %d points, %d positive bins, frame %u bytes",
             FFT_SIZE, FFT_HALF, (unsigned)DSP_FRAME_BYTES);
    return ESP_OK;
}

esp_err_t dsp_pipeline_process(const int16_t *audio,
                               uint8_t       *out_buf,
                               size_t         out_buf_size,
                               size_t        *out_len)
{
    if (!audio || !out_buf || out_buf_size < DSP_FRAME_BYTES) {
        return ESP_ERR_INVALID_ARG;
    }

    // ------------------------------------------------------------------
    // 1. Convert int16 -> float in [-1, 1) and apply the Hann window.
    //    The same pass packs the result as interleaved complex
    //    [re, im, re, im, ...] with imaginary parts zero, ready for the
    //    in-place radix-2 routine.
    // ------------------------------------------------------------------
    const float scale = 1.0f / 32768.0f;
    for (int i = 0; i < FFT_SIZE; ++i) {
        float s = (float)audio[i] * scale * s_window[i];
        s_complex[2 * i + 0] = s;
        s_complex[2 * i + 1] = 0.0f;
    }

    // ------------------------------------------------------------------
    // 2. Forward FFT (in-place) + bit-reversal to natural order.
    // ------------------------------------------------------------------
    dsps_fft2r_fc32(s_complex, FFT_SIZE);
    dsps_bit_rev_fc32(s_complex, FFT_SIZE);

    // ------------------------------------------------------------------
    // 3. Magnitude per positive bin and peak search.
    //    DC bin (i = 0) is skipped from peak detection because it just
    //    reflects DC offset of the mic, not "signal".
    // ------------------------------------------------------------------
    int   peak_bin = 1;
    float peak_mag = 0.0f;

    for (int i = 0; i < FFT_HALF; ++i) {
        float re = s_complex[2 * i + 0];
        float im = s_complex[2 * i + 1];
        float m  = sqrtf(re * re + im * im);
        s_mag[i] = m;
        if (i > 0 && m > peak_mag) {
            peak_mag = m;
            peak_bin = i;
        }
    }

    const uint32_t sr        = audio_capture_sample_rate();
    const float    bin_hz    = (float)sr / (float)FFT_SIZE;
    const float    peak_hz_f = (float)peak_bin * bin_hz;
    const float    peak_db_f = 20.0f * log10f(peak_mag + 1e-9f);

    // ------------------------------------------------------------------
    // 4. Build the on-wire frame.
    // ------------------------------------------------------------------
    dsp_frame_header_t hdr = {
        .magic       = DSP_MAGIC,
        .seq         = ++s_seq,
        .peak_hz     = peak_hz_f,
        .peak_db     = peak_db_f,
        .sample_rate = (uint16_t)sr,
        .n_wave      = DSP_WAVE_LEN,
        .n_spec      = DSP_SPEC_BINS,
        .reserved    = 0,
    };
    memcpy(out_buf, &hdr, sizeof(hdr));

    // 4a. Wave decimation: average groups of (FFT_SIZE / DSP_WAVE_LEN)
    //     samples for a cheap anti-aliased thumbnail of the waveform.
    int16_t *wave_out = (int16_t *)(out_buf + sizeof(hdr));
    const int group   = FFT_SIZE / DSP_WAVE_LEN;
    for (int i = 0; i < DSP_WAVE_LEN; ++i) {
        int32_t acc = 0;
        for (int j = 0; j < group; ++j) {
            acc += audio[i * group + j];
        }
        wave_out[i] = (int16_t)(acc / group);
    }

    // 4b. Spectrum: convert to dB FS, downsample to DSP_SPEC_BINS,
    //     clamp to [-128, 0] so the value fits an int8 (we ship the
    //     full byte range so the browser side can apply a colour map).
    int8_t *spec_out = (int8_t *)(out_buf + sizeof(hdr) + DSP_WAVE_LEN * sizeof(int16_t));
    const int sgrp   = FFT_HALF / DSP_SPEC_BINS;
    for (int i = 0; i < DSP_SPEC_BINS; ++i) {
        float sum = 0.0f;
        for (int j = 0; j < sgrp; ++j) {
            sum += s_mag[i * sgrp + j];
        }
        const float avg_mag = sum / (float)sgrp;
        float       db      = 20.0f * log10f(avg_mag + 1e-9f);
        if (db >  0.0f)  db = 0.0f;
        if (db < -128.0f) db = -128.0f;
        spec_out[i] = (int8_t)lrintf(db);
    }

    if (out_len) {
        *out_len = DSP_FRAME_BYTES;
    }
    return ESP_OK;
}
