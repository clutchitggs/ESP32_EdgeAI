#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start the HTTP server.
 *
 * Registers two URI handlers:
 *   - GET /     -> serves the embedded dashboard (index.html)
 *   - GET /ws   -> upgrades to a WebSocket and registers the client for
 *                  real-time analysis frame broadcast.
 *
 * The server task lives inside esp_http_server's own thread.
 */
esp_err_t web_server_start(void);

/**
 * @brief Send a binary frame to every connected WebSocket client.
 *
 * Safe to call from any task. Clients that fail a send are removed from
 * the broadcast list.
 *
 * @param data Pointer to the binary payload.
 * @param len  Length of the payload in bytes.
 */
void web_server_broadcast(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif
