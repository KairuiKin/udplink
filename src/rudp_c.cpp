#include "rudp/rudp_c.h"

#include "rudp/rudp.hpp"

#include <new>

struct rudp_endpoint_handle {
    rudp::Endpoint endpoint;
    rudp_hooks_v1 hooks_c;
    rudp::Hooks hooks_cpp;
    bool initialized;
};

namespace {

static rudp::Config ToCppConfig(const rudp_config_v1& src) {
    rudp::Config dst;
    dst.mtu = src.mtu;
    dst.max_payload = src.max_payload;
    dst.send_window = src.send_window;
    dst.recv_window = src.recv_window;
    dst.retransmit_min_ms = src.retransmit_min_ms;
    dst.retransmit_max_ms = src.retransmit_max_ms;
    dst.ack_delay_ms = src.ack_delay_ms;
    dst.connect_retry_ms = src.connect_retry_ms;
    dst.idle_timeout_ms = src.idle_timeout_ms;
    dst.heartbeat_ms = src.heartbeat_ms;
    dst.pacing_bytes_per_tick = src.pacing_bytes_per_tick;
    dst.key_update_retry_ms = src.key_update_retry_ms;
    dst.max_retransmits = src.max_retransmits;
    dst.max_connect_retries = src.max_connect_retries;
    dst.key_update_max_retries = src.key_update_max_retries;
    dst.enable_auth = src.enable_auth != 0;
    dst.auth_psk = src.auth_psk;
    dst.auth_key0 = src.auth_key0;
    dst.auth_key1 = src.auth_key1;
    return dst;
}

static void ToCConfig(const rudp::Config& src, rudp_config_v1* dst) {
    dst->mtu = src.mtu;
    dst->max_payload = src.max_payload;
    dst->send_window = src.send_window;
    dst->recv_window = src.recv_window;
    dst->retransmit_min_ms = src.retransmit_min_ms;
    dst->retransmit_max_ms = src.retransmit_max_ms;
    dst->ack_delay_ms = src.ack_delay_ms;
    dst->connect_retry_ms = src.connect_retry_ms;
    dst->idle_timeout_ms = src.idle_timeout_ms;
    dst->heartbeat_ms = src.heartbeat_ms;
    dst->pacing_bytes_per_tick = src.pacing_bytes_per_tick;
    dst->key_update_retry_ms = src.key_update_retry_ms;
    dst->max_retransmits = src.max_retransmits;
    dst->max_connect_retries = src.max_connect_retries;
    dst->key_update_max_retries = src.key_update_max_retries;
    dst->enable_auth = src.enable_auth ? 1u : 0u;
    dst->auth_psk = src.auth_psk;
    dst->auth_key0 = src.auth_key0;
    dst->auth_key1 = src.auth_key1;
}

static void ToCStats(const rudp::Stats& src, rudp_stats_v1* dst) {
    dst->tx_packets = src.tx_packets;
    dst->tx_retransmits = src.tx_retransmits;
    dst->tx_acks = src.tx_acks;
    dst->tx_control = src.tx_control;
    dst->tx_dropped_by_pacing = src.tx_dropped_by_pacing;
    dst->rx_packets = src.rx_packets;
    dst->rx_dropped = src.rx_dropped;
    dst->rx_delivered = src.rx_delivered;
    dst->rx_duplicates = src.rx_duplicates;
    dst->rx_control = src.rx_control;
    dst->connect_attempts = src.connect_attempts;
    dst->connect_success = src.connect_success;
    dst->connect_timeouts = src.connect_timeouts;
}

static void ToCMetrics(const rudp::RuntimeMetrics& src, rudp_runtime_metrics_v1* dst) {
    dst->pending_send = src.pending_send;
    dst->rto_ms = src.rto_ms;
    dst->smoothed_rtt_ms = src.smoothed_rtt_ms;
    dst->rtt_var_ms = src.rtt_var_ms;
    dst->tx_retransmit_ratio_permille = src.tx_retransmit_ratio_permille;
    dst->rx_drop_ratio_permille = src.rx_drop_ratio_permille;
}

static rudp_status MapSendStatus(rudp::SendStatus st) {
    switch (st) {
        case rudp::SendStatus::kOk:
            return RUDP_STATUS_OK;
        case rudp::SendStatus::kQueueFull:
            return RUDP_STATUS_QUEUE_FULL;
        case rudp::SendStatus::kPayloadTooLarge:
            return RUDP_STATUS_PAYLOAD_TOO_LARGE;
        case rudp::SendStatus::kSequenceSpaceExhausted:
            return RUDP_STATUS_SEQUENCE_SPACE_EXHAUSTED;
        case rudp::SendStatus::kNotConnected:
            return RUDP_STATUS_NOT_CONNECTED;
        case rudp::SendStatus::kUnsupported:
            return RUDP_STATUS_UNSUPPORTED;
        default:
            return RUDP_STATUS_INTERNAL_ERROR;
    }
}

static uint32_t NowMsThunk(void* user) {
    const rudp_endpoint_handle* handle = static_cast<const rudp_endpoint_handle*>(user);
    return handle->hooks_c.now_ms(handle->hooks_c.user);
}

static bool SendRawThunk(void* user, const uint8_t* data, uint16_t len) {
    const rudp_endpoint_handle* handle = static_cast<const rudp_endpoint_handle*>(user);
    return handle->hooks_c.send_raw(handle->hooks_c.user, data, len) != 0;
}

static void OnDeliverThunk(void* user, const uint8_t* data, uint16_t len) {
    const rudp_endpoint_handle* handle = static_cast<const rudp_endpoint_handle*>(user);
    handle->hooks_c.on_deliver(handle->hooks_c.user, data, len);
}

static bool IsInitialized(const rudp_endpoint_handle* handle) {
    return handle != 0 && handle->initialized;
}

}  // namespace

extern "C" {

const char* rudp_version_string(void) {
    return RUDP_VERSION_STRING;
}

uint32_t rudp_abi_version(void) {
    return RUDP_ABI_VERSION;
}

void rudp_default_config_v1(rudp_config_v1* out_config) {
    if (!out_config) {
        return;
    }
    ToCConfig(rudp::DefaultConfig(), out_config);
}

rudp_status rudp_config_for_profile_v1(uint8_t profile, rudp_config_v1* out_config) {
    if (!out_config) {
        return RUDP_STATUS_INVALID_ARGUMENT;
    }
    if (profile > static_cast<uint8_t>(RUDP_CONFIG_PROFILE_LOSSY_LINK)) {
        return RUDP_STATUS_INVALID_ARGUMENT;
    }
    const rudp::Config cfg = rudp::ConfigForProfile(static_cast<rudp::ConfigProfile>(profile));
    ToCConfig(cfg, out_config);
    return RUDP_STATUS_OK;
}

rudp_status rudp_endpoint_create(rudp_endpoint_handle** out_handle) {
    if (!out_handle) {
        return RUDP_STATUS_INVALID_ARGUMENT;
    }
    rudp_endpoint_handle* handle = new (std::nothrow) rudp_endpoint_handle();
    if (!handle) {
        *out_handle = 0;
        return RUDP_STATUS_INTERNAL_ERROR;
    }
    handle->hooks_c.user = 0;
    handle->hooks_c.now_ms = 0;
    handle->hooks_c.send_raw = 0;
    handle->hooks_c.on_deliver = 0;
    handle->hooks_cpp.user = handle;
    handle->hooks_cpp.now_ms = NowMsThunk;
    handle->hooks_cpp.send_raw = SendRawThunk;
    handle->hooks_cpp.send_raw_vec = 0;
    handle->hooks_cpp.on_deliver = OnDeliverThunk;
    handle->hooks_cpp.enter_critical = 0;
    handle->hooks_cpp.leave_critical = 0;
    handle->initialized = false;
    *out_handle = handle;
    return RUDP_STATUS_OK;
}

void rudp_endpoint_destroy(rudp_endpoint_handle* handle) {
    delete handle;
}

rudp_status rudp_endpoint_init(rudp_endpoint_handle* handle,
                               const rudp_config_v1* config,
                               const rudp_hooks_v1* hooks) {
    if (!handle || !config || !hooks || !hooks->now_ms || !hooks->send_raw || !hooks->on_deliver) {
        return RUDP_STATUS_INVALID_ARGUMENT;
    }
    handle->hooks_c = *hooks;
    handle->initialized = false;
    if (!handle->endpoint.Init(ToCppConfig(*config), handle->hooks_cpp)) {
        return RUDP_STATUS_INVALID_ARGUMENT;
    }
    handle->initialized = true;
    return RUDP_STATUS_OK;
}

rudp_status rudp_endpoint_start_connect(rudp_endpoint_handle* handle) {
    if (!handle) {
        return RUDP_STATUS_INVALID_ARGUMENT;
    }
    if (!handle->initialized) {
        return RUDP_STATUS_NOT_INITIALIZED;
    }
    return handle->endpoint.StartConnect() ? RUDP_STATUS_OK : RUDP_STATUS_INTERNAL_ERROR;
}

rudp_status rudp_endpoint_disconnect(rudp_endpoint_handle* handle) {
    if (!handle) {
        return RUDP_STATUS_INVALID_ARGUMENT;
    }
    if (!handle->initialized) {
        return RUDP_STATUS_NOT_INITIALIZED;
    }
    handle->endpoint.Disconnect();
    return RUDP_STATUS_OK;
}

rudp_status rudp_endpoint_on_udp_packet(rudp_endpoint_handle* handle,
                                        const uint8_t* data,
                                        uint16_t len) {
    if (!handle || (!data && len != 0)) {
        return RUDP_STATUS_INVALID_ARGUMENT;
    }
    if (!handle->initialized) {
        return RUDP_STATUS_NOT_INITIALIZED;
    }
    handle->endpoint.OnUdpPacket(data, len);
    return RUDP_STATUS_OK;
}

rudp_status rudp_endpoint_tick(rudp_endpoint_handle* handle) {
    if (!handle) {
        return RUDP_STATUS_INVALID_ARGUMENT;
    }
    if (!handle->initialized) {
        return RUDP_STATUS_NOT_INITIALIZED;
    }
    handle->endpoint.Tick();
    return RUDP_STATUS_OK;
}

rudp_status rudp_endpoint_send(rudp_endpoint_handle* handle,
                               const uint8_t* payload,
                               uint16_t len) {
    if (!handle || (!payload && len != 0)) {
        return RUDP_STATUS_INVALID_ARGUMENT;
    }
    if (!handle->initialized) {
        return RUDP_STATUS_NOT_INITIALIZED;
    }
    return MapSendStatus(handle->endpoint.Send(payload, len));
}

rudp_status rudp_endpoint_is_connected(const rudp_endpoint_handle* handle,
                                       int* out_connected) {
    if (!handle || !out_connected) {
        return RUDP_STATUS_INVALID_ARGUMENT;
    }
    if (!IsInitialized(handle)) {
        return RUDP_STATUS_NOT_INITIALIZED;
    }
    *out_connected = handle->endpoint.IsConnected() ? 1 : 0;
    return RUDP_STATUS_OK;
}

rudp_status rudp_endpoint_get_connection_state(const rudp_endpoint_handle* handle,
                                               uint8_t* out_state) {
    if (!handle || !out_state) {
        return RUDP_STATUS_INVALID_ARGUMENT;
    }
    if (!IsInitialized(handle)) {
        return RUDP_STATUS_NOT_INITIALIZED;
    }
    *out_state = static_cast<uint8_t>(handle->endpoint.GetConnectionState());
    return RUDP_STATUS_OK;
}

rudp_status rudp_endpoint_get_stats(const rudp_endpoint_handle* handle,
                                    rudp_stats_v1* out_stats) {
    if (!handle || !out_stats) {
        return RUDP_STATUS_INVALID_ARGUMENT;
    }
    if (!IsInitialized(handle)) {
        return RUDP_STATUS_NOT_INITIALIZED;
    }
    ToCStats(handle->endpoint.GetStats(), out_stats);
    return RUDP_STATUS_OK;
}

rudp_status rudp_endpoint_get_runtime_metrics(const rudp_endpoint_handle* handle,
                                              rudp_runtime_metrics_v1* out_metrics) {
    if (!handle || !out_metrics) {
        return RUDP_STATUS_INVALID_ARGUMENT;
    }
    if (!IsInitialized(handle)) {
        return RUDP_STATUS_NOT_INITIALIZED;
    }
    ToCMetrics(handle->endpoint.GetRuntimeMetrics(), out_metrics);
    return RUDP_STATUS_OK;
}

}  // extern "C"
