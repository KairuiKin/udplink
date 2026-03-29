#include "rudp/rudp_c.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct AppWire {
    uint32_t now_ms;
    uint32_t delivered;
} AppWire;

static uint32_t NowMs(void* user) {
    AppWire* wire = (AppWire*)user;
    return wire != 0 ? wire->now_ms : 0;
}

static int SendRaw(void* user, const uint8_t* data, uint16_t len) {
    (void)user;
    (void)data;
    return len > 0 ? 1 : 0;
}

static void OnDeliver(void* user, const uint8_t* data, uint16_t len) {
    AppWire* wire = (AppWire*)user;
    if (wire != 0 && data != 0 && len > 0) {
        ++wire->delivered;
    }
}

int main(void) {
    rudp_endpoint_handle* handle = 0;
    rudp_config_v1 cfg;
    rudp_hooks_v1 hooks;
    rudp_runtime_metrics_v1 metrics;
    AppWire wire;
    int connected = 0;
    uint8_t state = 0;

    wire.now_ms = 0;
    wire.delivered = 0;

    if (rudp_abi_version() != RUDP_ABI_VERSION) {
        fprintf(stderr, "unexpected ABI version\n");
        return 1;
    }

    printf("udplink version: %s\n", rudp_version_string());

    rudp_default_config_v1(&cfg);

    hooks.user = &wire;
    hooks.now_ms = NowMs;
    hooks.send_raw = SendRaw;
    hooks.on_deliver = OnDeliver;

    if (rudp_endpoint_create(&handle) != RUDP_STATUS_OK) {
        fprintf(stderr, "rudp_endpoint_create failed\n");
        return 2;
    }

    if (rudp_endpoint_init(handle, &cfg, &hooks) != RUDP_STATUS_OK) {
        fprintf(stderr, "rudp_endpoint_init failed\n");
        rudp_endpoint_destroy(handle);
        return 3;
    }

    if (rudp_endpoint_get_connection_state(handle, &state) != RUDP_STATUS_OK) {
        fprintf(stderr, "rudp_endpoint_get_connection_state failed\n");
        rudp_endpoint_destroy(handle);
        return 4;
    }

    if (rudp_endpoint_is_connected(handle, &connected) != RUDP_STATUS_OK) {
        fprintf(stderr, "rudp_endpoint_is_connected failed\n");
        rudp_endpoint_destroy(handle);
        return 5;
    }

    if (rudp_endpoint_get_runtime_metrics(handle, &metrics) != RUDP_STATUS_OK) {
        fprintf(stderr, "rudp_endpoint_get_runtime_metrics failed\n");
        rudp_endpoint_destroy(handle);
        return 6;
    }

    printf("initial state=%u connected=%d pending_send=%u\n",
           (unsigned)state,
           connected,
           (unsigned)metrics.pending_send);

    if (rudp_endpoint_send(handle, (const uint8_t*)"demo", 5) != RUDP_STATUS_NOT_CONNECTED) {
        fprintf(stderr, "expected NOT_CONNECTED before handshake\n");
        rudp_endpoint_destroy(handle);
        return 7;
    }

    rudp_endpoint_destroy(handle);
    printf("example completed\n");
    return 0;
}