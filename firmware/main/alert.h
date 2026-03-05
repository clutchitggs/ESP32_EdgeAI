#pragma once

/**
 * @brief Initialize hardware used for alerts (buzzer/LED).
 *
 * In the initial version, this may be a stub or only configure a
 * simple GPIO pin. Later this can be extended to support PWM buzzer,
 * RGB LED, or other indicators.
 */
void alert_init(void);

/**
 * @brief Trigger a drowsiness alert.
 *
 * The concrete implementation can range from toggling an LED to
 * generating an audible buzzer pattern.
 */
void alert_trigger(void);

/**
 * @brief Clear or stop any active alert.
 */
void alert_clear(void);

