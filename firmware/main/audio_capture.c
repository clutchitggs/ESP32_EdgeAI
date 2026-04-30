#include "audio_capture.h"

#include "driver/i2s_pdm.h"
#include "esp_log.h"
#include "sdkconfig.h"

static const char *TAG = "audio_capture";

// XIAO ESP32-S3 Sense on-board microphone routing.
#define PDM_CLK_GPIO  GPIO_NUM_42
#define PDM_DIN_GPIO  GPIO_NUM_41

static i2s_chan_handle_t s_rx_chan;
static uint32_t          s_sample_rate;

esp_err_t audio_capture_init(void)
{
    s_sample_rate = (uint32_t)CONFIG_AUDIO_SAMPLE_RATE_HZ;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    // Larger DMA buffers smooth out scheduling jitter; ~30 ms of audio at 16 kHz.
    chan_cfg.dma_desc_num  = 6;
    chan_cfg.dma_frame_num = 240;

    esp_err_t err = i2s_new_channel(&chan_cfg, NULL, &s_rx_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_new_channel failed: %s", esp_err_to_name(err));
        return err;
    }

    i2s_pdm_rx_config_t pdm_rx_cfg = {
        .clk_cfg  = I2S_PDM_RX_CLK_DEFAULT_CONFIG(s_sample_rate),
        .slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                   I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .clk = PDM_CLK_GPIO,
            .din = PDM_DIN_GPIO,
            .invert_flags = {
                .clk_inv = false,
            },
        },
    };

    err = i2s_channel_init_pdm_rx_mode(s_rx_chan, &pdm_rx_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_init_pdm_rx_mode failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2s_channel_enable(s_rx_chan);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_enable failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "PDM mic ready @ %lu Hz (clk=GPIO%d, din=GPIO%d)",
             (unsigned long)s_sample_rate, PDM_CLK_GPIO, PDM_DIN_GPIO);
    return ESP_OK;
}

esp_err_t audio_capture_read(int16_t *dst, size_t n_samples, TickType_t timeout)
{
    if (!dst || n_samples == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t bytes_to_read = n_samples * sizeof(int16_t);
    size_t bytes_read    = 0;

    esp_err_t err = i2s_channel_read(s_rx_chan, dst, bytes_to_read, &bytes_read, timeout);
    if (err != ESP_OK) {
        return err;
    }
    if (bytes_read != bytes_to_read) {
        return ESP_ERR_TIMEOUT;
    }
    return ESP_OK;
}

uint32_t audio_capture_sample_rate(void)
{
    return s_sample_rate;
}
