#include "alert.h"

#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "alert";

// Placeholder GPIO for alert output (e.g., onboard LED or external buzzer).
// Adjust this pin to match your board wiring when you connect hardware.
static const gpio_num_t ALERT_GPIO = GPIO_NUM_2;

void alert_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << ALERT_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure alert GPIO: 0x%x", err);
        return;
    }

    gpio_set_level(ALERT_GPIO, 0);
    ESP_LOGI(TAG, "Alert output initialized on GPIO %d", ALERT_GPIO);
}

void alert_trigger(void)
{
    gpio_set_level(ALERT_GPIO, 1);
    ESP_LOGI(TAG, "Alert triggered");
}

void alert_clear(void)
{
    gpio_set_level(ALERT_GPIO, 0);
    ESP_LOGI(TAG, "Alert cleared");
}

