#include "rudp/rudp_c.h"

#include <stdio.h>
#include <string.h>
#include <vector>

#define CHECK_OR_RET(cond, code) \
    do { \
        if (!(cond)) { \
            printf("c_api_test failed: line=%d code=%d\n", __LINE__, (code)); \
            return (code); \
        } \
    } while (0)

struct Wire {
    uint32_t now_ms;
    std::vector<std::vector<uint8_t> > a2b;
    std::vector<std::vector<uint8_t> > b2a;
    std::vector<std::vector<uint8_t> > recv_a;
    std::vector<std::vector<uint8_t> > recv_b;
};

static uint32_t NowMs(void* user) {
    return static_cast<Wire*>(user)->now_ms;
}

static int SendA(void* user, const uint8_t* data, uint16_t len) {
    Wire* wire = static_cast<Wire*>(user);
    wire->a2b.push_back(std::vector<uint8_t>(data, data + len));
    return 1;
}

static int SendB(void* user, const uint8_t* data, uint16_t len) {
    Wire* wire = static_cast<Wire*>(user);
    wire->b2a.push_back(std::vector<uint8_t>(data, data + len));
    return 1;
}

static void DeliverA(void* user, const uint8_t* data, uint16_t len) {
    Wire* wire = static_cast<Wire*>(user);
    wire->recv_a.push_back(std::vector<uint8_t>(data, data + len));
}

static void DeliverB(void* user, const uint8_t* data, uint16_t len) {
    Wire* wire = static_cast<Wire*>(user);
    wire->recv_b.push_back(std::vector<uint8_t>(data, data + len));
}

int main() {
    CHECK_OR_RET(rudp_abi_version() == RUDP_ABI_VERSION, 10);
    CHECK_OR_RET(strcmp(rudp_version_string(), "0.1.2") == 0, 11);
    CHECK_OR_RET(rudp_config_for_profile_v1(255, 0) == RUDP_STATUS_INVALID_ARGUMENT, 12);

    rudp_endpoint_handle* a = 0;
    rudp_endpoint_handle* b = 0;
    rudp_endpoint_handle* uninit = 0;
    CHECK_OR_RET(rudp_endpoint_create(0) == RUDP_STATUS_INVALID_ARGUMENT, 35);
    CHECK_OR_RET(rudp_endpoint_create(&a) == RUDP_STATUS_OK, 36);
    CHECK_OR_RET(rudp_endpoint_create(&b) == RUDP_STATUS_OK, 37);
    CHECK_OR_RET(rudp_endpoint_create(&uninit) == RUDP_STATUS_OK, 38);

    rudp_config_v1 cfg;
    rudp_config_v1 low_latency_cfg;
    rudp_config_v1 low_power_cfg;
    rudp_config_v1 lossy_cfg;
    rudp_default_config_v1(&cfg);
    CHECK_OR_RET(cfg.max_payload > 0, 17);
    CHECK_OR_RET(rudp_config_for_profile_v1(RUDP_CONFIG_PROFILE_LOW_LATENCY, &low_latency_cfg) == RUDP_STATUS_OK, 18);
    CHECK_OR_RET(rudp_config_for_profile_v1(RUDP_CONFIG_PROFILE_LOW_POWER, &low_power_cfg) == RUDP_STATUS_OK, 19);
    CHECK_OR_RET(rudp_config_for_profile_v1(RUDP_CONFIG_PROFILE_LOSSY_LINK, &lossy_cfg) == RUDP_STATUS_OK, 20);
    CHECK_OR_RET(low_latency_cfg.ack_delay_ms < low_power_cfg.ack_delay_ms, 21);
    CHECK_OR_RET(low_power_cfg.heartbeat_ms > low_latency_cfg.heartbeat_ms, 22);
    CHECK_OR_RET(lossy_cfg.max_retransmits > low_latency_cfg.max_retransmits, 23);
    cfg = low_latency_cfg;
    cfg.mtu = 128;
    cfg.max_payload = 80;
    cfg.send_window = 16;
    cfg.recv_window = 16;
    cfg.retransmit_min_ms = 8;
    cfg.retransmit_max_ms = 120;
    cfg.connect_retry_ms = 20;
    cfg.max_connect_retries = 8;
    cfg.idle_timeout_ms = 2000;
    cfg.heartbeat_ms = 80;
    cfg.pacing_bytes_per_tick = 0;
    cfg.enable_auth = 1;
    cfg.auth_psk = 0xA53C19D1u;
    cfg.auth_key0 = 0x0706050403020100ull;
    cfg.auth_key1 = 0x0F0E0D0C0B0A0908ull;

    rudp_config_v1 bad_cfg = cfg;
    bad_cfg.max_payload = 0;

    Wire wire;
    wire.now_ms = 1;

    rudp_hooks_v1 ha = {&wire, NowMs, SendA, DeliverA};
    rudp_hooks_v1 hb = {&wire, NowMs, SendB, DeliverB};
    rudp_hooks_v1 bad_hooks = {&wire, 0, SendA, DeliverA};

    CHECK_OR_RET(rudp_endpoint_start_connect(0) == RUDP_STATUS_INVALID_ARGUMENT, 30);
    CHECK_OR_RET(rudp_endpoint_tick(0) == RUDP_STATUS_INVALID_ARGUMENT, 31);
    CHECK_OR_RET(rudp_endpoint_disconnect(0) == RUDP_STATUS_INVALID_ARGUMENT, 32);
    CHECK_OR_RET(rudp_endpoint_send(0, reinterpret_cast<const uint8_t*>("x"), 2) == RUDP_STATUS_INVALID_ARGUMENT, 33);
    int connected_probe = 0;
    uint8_t state_probe = 0;
    CHECK_OR_RET(rudp_endpoint_is_connected(0, &connected_probe) == RUDP_STATUS_INVALID_ARGUMENT, 34);
    CHECK_OR_RET(rudp_endpoint_get_connection_state(0, &state_probe) == RUDP_STATUS_INVALID_ARGUMENT, 35);

    CHECK_OR_RET(rudp_endpoint_init(0, &cfg, &ha) == RUDP_STATUS_INVALID_ARGUMENT, 36);
    CHECK_OR_RET(rudp_endpoint_init(a, 0, &ha) == RUDP_STATUS_INVALID_ARGUMENT, 37);
    CHECK_OR_RET(rudp_endpoint_init(a, &cfg, 0) == RUDP_STATUS_INVALID_ARGUMENT, 38);
    CHECK_OR_RET(rudp_endpoint_init(a, &cfg, &bad_hooks) == RUDP_STATUS_INVALID_ARGUMENT, 39);
    CHECK_OR_RET(rudp_endpoint_init(a, &bad_cfg, &ha) == RUDP_STATUS_INVALID_ARGUMENT, 40);
    CHECK_OR_RET(rudp_endpoint_init(a, &cfg, &ha) == RUDP_STATUS_OK, 41);
    CHECK_OR_RET(rudp_endpoint_init(b, &cfg, &hb) == RUDP_STATUS_OK, 42);

    int connected = 0;
    uint8_t state = 0;
    rudp_stats_v1 stats;
    rudp_runtime_metrics_v1 metrics;
    CHECK_OR_RET(rudp_endpoint_start_connect(uninit) == RUDP_STATUS_NOT_INITIALIZED, 43);
    CHECK_OR_RET(rudp_endpoint_tick(uninit) == RUDP_STATUS_NOT_INITIALIZED, 44);
    CHECK_OR_RET(rudp_endpoint_disconnect(uninit) == RUDP_STATUS_NOT_INITIALIZED, 45);
    CHECK_OR_RET(rudp_endpoint_send(uninit, reinterpret_cast<const uint8_t*>("x"), 2) == RUDP_STATUS_NOT_INITIALIZED, 46);
    CHECK_OR_RET(rudp_endpoint_on_udp_packet(uninit, reinterpret_cast<const uint8_t*>("x"), 1) == RUDP_STATUS_NOT_INITIALIZED, 47);
    CHECK_OR_RET(rudp_endpoint_is_connected(uninit, &connected) == RUDP_STATUS_NOT_INITIALIZED, 48);
    CHECK_OR_RET(rudp_endpoint_get_connection_state(uninit, &state) == RUDP_STATUS_NOT_INITIALIZED, 49);
    CHECK_OR_RET(rudp_endpoint_get_stats(uninit, &stats) == RUDP_STATUS_NOT_INITIALIZED, 50);
    CHECK_OR_RET(rudp_endpoint_get_runtime_metrics(uninit, &metrics) == RUDP_STATUS_NOT_INITIALIZED, 51);

    CHECK_OR_RET(rudp_endpoint_start_connect(a) == RUDP_STATUS_OK, 52);

    for (int i = 0; i < 2000; ++i) {
        ++wire.now_ms;
        CHECK_OR_RET(rudp_endpoint_tick(a) == RUDP_STATUS_OK, 53);
        CHECK_OR_RET(rudp_endpoint_tick(b) == RUDP_STATUS_OK, 54);
        while (!wire.a2b.empty()) {
            std::vector<uint8_t> pkt = wire.a2b.back();
            wire.a2b.pop_back();
            CHECK_OR_RET(rudp_endpoint_on_udp_packet(b, &pkt[0], static_cast<uint16_t>(pkt.size())) == RUDP_STATUS_OK, 55);
        }
        while (!wire.b2a.empty()) {
            std::vector<uint8_t> pkt = wire.b2a.back();
            wire.b2a.pop_back();
            CHECK_OR_RET(rudp_endpoint_on_udp_packet(a, &pkt[0], static_cast<uint16_t>(pkt.size())) == RUDP_STATUS_OK, 56);
        }
        int connected_a = 0;
        int connected_b = 0;
        CHECK_OR_RET(rudp_endpoint_is_connected(a, &connected_a) == RUDP_STATUS_OK, 57);
        CHECK_OR_RET(rudp_endpoint_is_connected(b, &connected_b) == RUDP_STATUS_OK, 58);
        if (connected_a && connected_b) {
            break;
        }
    }

    int connected_a = 0;
    int connected_b = 0;
    uint8_t state_a = 0;
    rudp_stats_v1 stats_a;
    rudp_runtime_metrics_v1 metrics_a;
    CHECK_OR_RET(rudp_endpoint_is_connected(0, &connected_a) == RUDP_STATUS_INVALID_ARGUMENT, 59);
    CHECK_OR_RET(rudp_endpoint_get_connection_state(a, 0) == RUDP_STATUS_INVALID_ARGUMENT, 60);
    CHECK_OR_RET(rudp_endpoint_get_stats(a, 0) == RUDP_STATUS_INVALID_ARGUMENT, 61);
    CHECK_OR_RET(rudp_endpoint_get_runtime_metrics(a, 0) == RUDP_STATUS_INVALID_ARGUMENT, 62);
    CHECK_OR_RET(rudp_endpoint_on_udp_packet(a, 0, 1) == RUDP_STATUS_INVALID_ARGUMENT, 63);
    CHECK_OR_RET(rudp_endpoint_send(a, 0, 1) == RUDP_STATUS_INVALID_ARGUMENT, 64);
    CHECK_OR_RET(rudp_endpoint_is_connected(a, &connected_a) == RUDP_STATUS_OK, 65);
    CHECK_OR_RET(rudp_endpoint_is_connected(b, &connected_b) == RUDP_STATUS_OK, 66);
    CHECK_OR_RET(connected_a == 1 && connected_b == 1, 67);
    CHECK_OR_RET(rudp_endpoint_get_connection_state(a, &state_a) == RUDP_STATUS_OK, 68);
    CHECK_OR_RET(state_a == static_cast<uint8_t>(RUDP_CONNECTION_CONNECTED), 69);
    CHECK_OR_RET(rudp_endpoint_get_stats(a, &stats_a) == RUDP_STATUS_OK, 70);
    CHECK_OR_RET(rudp_endpoint_get_runtime_metrics(a, &metrics_a) == RUDP_STATUS_OK, 71);
    CHECK_OR_RET(stats_a.connect_success > 0, 72);
    CHECK_OR_RET(metrics_a.rto_ms >= cfg.retransmit_min_ms, 73);

    const char* msg = "c-abi-message";
    CHECK_OR_RET(rudp_endpoint_send(a, reinterpret_cast<const uint8_t*>(msg), static_cast<uint16_t>(strlen(msg) + 1)) == RUDP_STATUS_OK, 74);
    for (int i = 0; i < 2000; ++i) {
        ++wire.now_ms;
        CHECK_OR_RET(rudp_endpoint_tick(a) == RUDP_STATUS_OK, 75);
        CHECK_OR_RET(rudp_endpoint_tick(b) == RUDP_STATUS_OK, 76);
        while (!wire.a2b.empty()) {
            std::vector<uint8_t> pkt = wire.a2b.back();
            wire.a2b.pop_back();
            CHECK_OR_RET(rudp_endpoint_on_udp_packet(b, &pkt[0], static_cast<uint16_t>(pkt.size())) == RUDP_STATUS_OK, 77);
        }
        while (!wire.b2a.empty()) {
            std::vector<uint8_t> pkt = wire.b2a.back();
            wire.b2a.pop_back();
            CHECK_OR_RET(rudp_endpoint_on_udp_packet(a, &pkt[0], static_cast<uint16_t>(pkt.size())) == RUDP_STATUS_OK, 78);
        }
        if (!wire.recv_b.empty()) {
            break;
        }
    }
    CHECK_OR_RET(!wire.recv_b.empty(), 79);
    CHECK_OR_RET(strcmp(reinterpret_cast<const char*>(&wire.recv_b[0][0]), msg) == 0, 80);

    CHECK_OR_RET(rudp_endpoint_disconnect(a) == RUDP_STATUS_OK, 81);
    CHECK_OR_RET(rudp_endpoint_get_connection_state(a, &state_a) == RUDP_STATUS_OK, 82);
    CHECK_OR_RET(state_a == static_cast<uint8_t>(RUDP_CONNECTION_DISCONNECTED), 83);
    CHECK_OR_RET(rudp_endpoint_send(a, reinterpret_cast<const uint8_t*>(msg), static_cast<uint16_t>(strlen(msg) + 1)) == RUDP_STATUS_NOT_CONNECTED, 84);

    rudp_endpoint_destroy(uninit);
    rudp_endpoint_destroy(a);
    rudp_endpoint_destroy(b);
    puts("rudp c_api_test passed");
    return 0;
}
