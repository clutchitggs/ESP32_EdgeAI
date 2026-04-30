#include "web_server.h"

#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "web_server";

// Up to MAX_CLIENTS browsers can listen at once. The dashboard is a single-page
// app so this is plenty even with a few tabs open during a demo.
#define MAX_CLIENTS 4

static httpd_handle_t      s_server;
static int                 s_clients[MAX_CLIENTS];
static SemaphoreHandle_t   s_clients_mu;

// Web dashboard, embedded as a text blob via EMBED_TXTFILES in CMakeLists.txt.
extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[]   asm("_binary_index_html_end");

// -----------------------------------------------------------------------------
// Client list helpers
// -----------------------------------------------------------------------------
static void client_add(int fd)
{
    xSemaphoreTake(s_clients_mu, portMAX_DELAY);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (s_clients[i] == fd) {  // already known
            xSemaphoreGive(s_clients_mu);
            return;
        }
    }
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (s_clients[i] == -1) {
            s_clients[i] = fd;
            ESP_LOGI(TAG, "WS client connected (fd=%d, slot=%d)", fd, i);
            xSemaphoreGive(s_clients_mu);
            return;
        }
    }
    ESP_LOGW(TAG, "WS client list full, dropping fd=%d", fd);
    xSemaphoreGive(s_clients_mu);
}

static void client_remove(int fd)
{
    xSemaphoreTake(s_clients_mu, portMAX_DELAY);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (s_clients[i] == fd) {
            s_clients[i] = -1;
            ESP_LOGI(TAG, "WS client disconnected (fd=%d)", fd);
            break;
        }
    }
    xSemaphoreGive(s_clients_mu);
}

// -----------------------------------------------------------------------------
// HTTP / WebSocket handlers
// -----------------------------------------------------------------------------
static esp_err_t index_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    // EMBED_TXTFILES appends a NUL byte; subtract it from the length.
    const size_t len = (index_html_end - index_html_start) - 1;
    return httpd_resp_send(req, index_html_start, len);
}

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        // First call is the upgrade handshake — register the client.
        client_add(httpd_req_to_sockfd(req));
        return ESP_OK;
    }

    // Subsequent frames from the browser. We don't accept any commands today,
    // but we still drain incoming frames so the connection stays healthy.
    httpd_ws_frame_t frame = { .type = HTTPD_WS_TYPE_TEXT };
    uint8_t buf[128];
    frame.payload = buf;
    return httpd_ws_recv_frame(req, &frame, sizeof(buf));
}

static const httpd_uri_t s_uri_index = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = index_get_handler,
    .user_ctx = NULL,
};

static const httpd_uri_t s_uri_ws = {
    .uri          = "/ws",
    .method       = HTTP_GET,
    .handler      = ws_handler,
    .user_ctx     = NULL,
    .is_websocket = true,
};

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------
esp_err_t web_server_start(void)
{
    s_clients_mu = xSemaphoreCreateMutex();
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        s_clients[i] = -1;
    }

    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.max_open_sockets = MAX_CLIENTS + 2;  // +1 for index, +1 buffer
    cfg.stack_size       = 8192;

    esp_err_t err = httpd_start(&s_server, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        return err;
    }

    httpd_register_uri_handler(s_server, &s_uri_index);
    httpd_register_uri_handler(s_server, &s_uri_ws);

    ESP_LOGI(TAG, "HTTP server listening on port %d", cfg.server_port);
    return ESP_OK;
}

void web_server_broadcast(const uint8_t *data, size_t len)
{
    if (!s_server || !data || len == 0) {
        return;
    }

    httpd_ws_frame_t frame = {
        .type    = HTTPD_WS_TYPE_BINARY,
        .payload = (uint8_t *)data,
        .len     = len,
    };

    // Snapshot the client list so we hold the mutex briefly.
    int snapshot[MAX_CLIENTS];
    xSemaphoreTake(s_clients_mu, portMAX_DELAY);
    memcpy(snapshot, s_clients, sizeof(snapshot));
    xSemaphoreGive(s_clients_mu);

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        int fd = snapshot[i];
        if (fd < 0) continue;

        esp_err_t err = httpd_ws_send_frame_async(s_server, fd, &frame);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "WS send to fd=%d failed: %s — dropping",
                     fd, esp_err_to_name(err));
            client_remove(fd);
        }
    }
}
