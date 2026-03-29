#include "rudp/rudp_c.h"

#include <stdint.h>
#include <string.h>

typedef struct AppWire {
    uint32_t now_ms;
    uint32_t delivered;
} AppWire;

static uint32_t NowMs(void* user) {
    return ((AppWire*)user)->now_ms;
}

static int SendRaw(void* user, const uint8_t* data, uint16_t len) {
    (void)user;
    (void)data;
    return len > 0 ? 1 : 0;
}

static void OnDeliver(void* user, const uint8_t* data, uint16_t len) {
    AppWire* wire = (AppWire*)user;
    if (data != 0 && len > 0) {
        ++wire->delivered;
    }
}

int main(void) {
    rudp_endpoint_handle* handle = 0;
    rudp_config_v1 cfg;
    rudp_hooks_v1 hooks;
    AppWire wire;
    int connected = 0;
    uint8_t state = 0;
    rudp_runtime_metrics_v1 metrics;

    wire.now_ms = 0;
    wire.delivered = 0;

    if (rudp_abi_version() != RUDP_ABI_VERSION) {
        return 1;
    }
    if (strcmp(rudp_version_string(), "0.1.2") != 0) {
        return 2;
    }
    rudp_default_config_v1(&cfg);
    hooks.user = &wire;
    hooks.now_ms = NowMs;
    hooks.send_raw = SendRaw;
    hooks.on_deliver = OnDeliver;

    if (rudp_endpoint_create(&handle) != RUDP_STATUS_OK) {
        return 3;
    }
    if (rudp_endpoint_init(handle, &cfg, &hooks) != RUDP_STATUS_OK) {
        rudp_endpoint_destroy(handle);
        return 4;
    }
    if (rudp_endpoint_get_connection_state(handle, &state) != RUDP_STATUS_OK) {
        rudp_endpoint_destroy(handle);
        return 5;
    }
    if (state != (uint8_t)RUDP_CONNECTION_DISCONNECTED) {
        rudp_endpoint_destroy(handle);
        return 6;
    }
    if (rudp_endpoint_is_connected(handle, &connected) != RUDP_STATUS_OK) {
        rudp_endpoint_destroy(handle);
        return 7;
    }
    if (connected != 0) {
        rudp_endpoint_destroy(handle);
        return 8;
    }
    if (rudp_endpoint_get_runtime_metrics(handle, &metrics) != RUDP_STATUS_OK) {
        rudp_endpoint_destroy(handle);
        return 9;
    }
    if (metrics.pending_send != 0) {
        rudp_endpoint_destroy(handle);
        return 10;
    }
    if (rudp_endpoint_send(handle, (const uint8_t*)"x", 2) != RUDP_STATUS_NOT_CONNECTED) {
        rudp_endpoint_destroy(handle);
        return 11;
    }

    rudp_endpoint_destroy(handle);
    return 0;
}
