#ifndef RUDP_RUDP_C_H
#define RUDP_RUDP_C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ABI version for the current C surface. Increment only when the exported
// binary interface changes incompatibly.
#define RUDP_ABI_VERSION 1u

// Opaque endpoint handle. Callers must not inspect or copy internal state.
typedef struct rudp_endpoint_handle rudp_endpoint_handle;

typedef enum rudp_status {
    RUDP_STATUS_OK = 0,
    RUDP_STATUS_INVALID_ARGUMENT = 1,
    RUDP_STATUS_NOT_INITIALIZED = 2,
    RUDP_STATUS_QUEUE_FULL = 3,
    RUDP_STATUS_PAYLOAD_TOO_LARGE = 4,
    RUDP_STATUS_NOT_CONNECTED = 5,
    RUDP_STATUS_UNSUPPORTED = 6,
    RUDP_STATUS_SEQUENCE_SPACE_EXHAUSTED = 7,
    RUDP_STATUS_INTERNAL_ERROR = 255
} rudp_status;

typedef enum rudp_connection_state {
    RUDP_CONNECTION_DISCONNECTED = 0,
    RUDP_CONNECTION_CONNECTING = 1,
    RUDP_CONNECTION_CONNECTED = 2,
    RUDP_CONNECTION_TIMED_OUT = 3
} rudp_connection_state;

typedef enum rudp_config_profile {
    RUDP_CONFIG_PROFILE_BALANCED = 0,
    RUDP_CONFIG_PROFILE_LOW_LATENCY = 1,
    RUDP_CONFIG_PROFILE_LOW_POWER = 2,
    RUDP_CONFIG_PROFILE_LOSSY_LINK = 3
} rudp_config_profile;

typedef struct rudp_stats_v1 {
    uint32_t tx_packets;
    uint32_t tx_retransmits;
    uint32_t tx_acks;
    uint32_t tx_control;
    uint32_t tx_dropped_by_pacing;
    uint32_t rx_packets;
    uint32_t rx_dropped;
    uint32_t rx_delivered;
    uint32_t rx_duplicates;
    uint32_t rx_control;
    uint32_t connect_attempts;
    uint32_t connect_success;
    uint32_t connect_timeouts;
} rudp_stats_v1;

typedef struct rudp_runtime_metrics_v1 {
    uint16_t pending_send;
    uint32_t rto_ms;
    uint32_t smoothed_rtt_ms;
    uint32_t rtt_var_ms;
    uint32_t tx_retransmit_ratio_permille;
    uint32_t rx_drop_ratio_permille;
} rudp_runtime_metrics_v1;

typedef struct rudp_config_v1 {
    uint16_t mtu;
    uint16_t max_payload;
    uint16_t send_window;
    uint16_t recv_window;
    uint16_t retransmit_min_ms;
    uint16_t retransmit_max_ms;
    uint16_t ack_delay_ms;
    uint16_t connect_retry_ms;
    uint16_t idle_timeout_ms;
    uint16_t heartbeat_ms;
    uint16_t pacing_bytes_per_tick;
    uint16_t key_update_retry_ms;
    uint8_t max_retransmits;
    uint8_t max_connect_retries;
    uint8_t key_update_max_retries;
    uint8_t enable_auth;
    uint32_t auth_psk;
    uint64_t auth_key0;
    uint64_t auth_key1;
} rudp_config_v1;

typedef uint32_t (*rudp_now_ms_fn)(void* user);
typedef int (*rudp_send_raw_fn)(void* user, const uint8_t* data, uint16_t len);
typedef void (*rudp_on_deliver_fn)(void* user, const uint8_t* data, uint16_t len);

typedef struct rudp_hooks_v1 {
    void* user;
    rudp_now_ms_fn now_ms;
    rudp_send_raw_fn send_raw;
    rudp_on_deliver_fn on_deliver;
} rudp_hooks_v1;

// Returns the library version string compiled into the current binary.
const char* rudp_version_string(void);
// Returns the ABI version of this header/implementation pair.
uint32_t rudp_abi_version(void);

// Fills a config with library defaults. Null is ignored.
void rudp_default_config_v1(rudp_config_v1* out_config);
// Fills a config from a predefined profile.
rudp_status rudp_config_for_profile_v1(uint8_t profile, rudp_config_v1* out_config);

// Creates an endpoint handle. Destroy it with rudp_endpoint_destroy().
rudp_status rudp_endpoint_create(rudp_endpoint_handle** out_handle);
void rudp_endpoint_destroy(rudp_endpoint_handle* handle);
// Initializes the handle. Required before start_connect/tick/send/query APIs.
rudp_status rudp_endpoint_init(rudp_endpoint_handle* handle,
                               const rudp_config_v1* config,
                               const rudp_hooks_v1* hooks);
rudp_status rudp_endpoint_start_connect(rudp_endpoint_handle* handle);
rudp_status rudp_endpoint_disconnect(rudp_endpoint_handle* handle);
rudp_status rudp_endpoint_on_udp_packet(rudp_endpoint_handle* handle,
                                        const uint8_t* data,
                                        uint16_t len);
rudp_status rudp_endpoint_tick(rudp_endpoint_handle* handle);
rudp_status rudp_endpoint_send(rudp_endpoint_handle* handle,
                               const uint8_t* payload,
                               uint16_t len);
rudp_status rudp_endpoint_is_connected(const rudp_endpoint_handle* handle,
                                       int* out_connected);
rudp_status rudp_endpoint_get_connection_state(const rudp_endpoint_handle* handle,
                                               uint8_t* out_state);
rudp_status rudp_endpoint_get_stats(const rudp_endpoint_handle* handle,
                                    rudp_stats_v1* out_stats);
rudp_status rudp_endpoint_get_runtime_metrics(const rudp_endpoint_handle* handle,
                                              rudp_runtime_metrics_v1* out_metrics);

// Threading note: one handle should be driven by one logical owner at a time.
// Concurrent tick/send/on_udp_packet/disconnect calls on the same handle are
// not supported unless the caller serializes access externally.

#ifdef __cplusplus
}
#endif

#endif
