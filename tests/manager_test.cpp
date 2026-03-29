#include "rudp/manager.hpp"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#define CHECK_OR_RET(cond, code) \
    do { \
        if (!(cond)) { \
            printf("manager_test failed: line=%d code=%d\n", __LINE__, (code)); \
            return (code); \
        } \
    } while (0)

struct Packet {
    uint64_t key;
    std::vector<uint8_t> data;
};

struct Delivered {
    uint64_t key;
    std::vector<uint8_t> data;
};

struct Node {
    uint32_t now_ms;
    std::vector<Packet> out;
    std::vector<Delivered> delivered;
    bool in_critical;
    bool reentered_critical;
};

static uint32_t NowMs(void* u) {
    return static_cast<Node*>(u)->now_ms;
}

static bool SendRaw(void* u, uint64_t key, const uint8_t* data, uint16_t len) {
    Node* n = static_cast<Node*>(u);
    Packet p;
    p.key = key;
    p.data.assign(data, data + len);
    n->out.push_back(p);
    return true;
}

static bool SendRawVec(void* u, uint64_t key,
                       const uint8_t* head, uint16_t head_len,
                       const uint8_t* body, uint16_t body_len) {
    Node* n = static_cast<Node*>(u);
    Packet p;
    p.key = key;
    p.data.reserve(static_cast<size_t>(head_len) + static_cast<size_t>(body_len));
    p.data.insert(p.data.end(), head, head + head_len);
    p.data.insert(p.data.end(), body, body + body_len);
    n->out.push_back(p);
    return true;
}

static void OnDeliver(void* u, uint64_t key, const uint8_t* data, uint16_t len) {
    Node* n = static_cast<Node*>(u);
    Delivered d;
    d.key = key;
    d.data.assign(data, data + len);
    n->delivered.push_back(d);
}

static void EnterCritical(void* u) {
    Node* n = static_cast<Node*>(u);
    if (n->in_critical) {
        n->reentered_critical = true;
    }
    n->in_critical = true;
}

static void LeaveCritical(void* u) {
    Node* n = static_cast<Node*>(u);
    n->in_critical = false;
}

int main() {
    Node a_node;
    Node b_node;
    a_node.now_ms = 1;
    b_node.now_ms = 1;
    a_node.in_critical = false;
    b_node.in_critical = false;
    a_node.reentered_critical = false;
    b_node.reentered_critical = false;

    rudp::Config cfg = rudp::DefaultConfig();
    cfg.mtu = 160;
    cfg.max_payload = 96;
    cfg.send_window = 16;
    cfg.recv_window = 16;
    cfg.enable_auth = true;
    cfg.auth_key0 = 0x0706050403020100ull;
    cfg.auth_key1 = 0x0F0E0D0C0B0A0908ull;

    rudp::ManagerHooks ha = {&a_node, NowMs, SendRaw, SendRawVec, OnDeliver, EnterCritical, LeaveCritical};
    rudp::ManagerHooks hb = {&b_node, NowMs, SendRaw, SendRawVec, OnDeliver, EnterCritical, LeaveCritical};

    static rudp::ConnectionManager a_mgr;
    static rudp::ConnectionManager b_mgr;
    CHECK_OR_RET(a_mgr.Init(cfg, ha), 10);
    CHECK_OR_RET(b_mgr.Init(cfg, hb), 11);

    CHECK_OR_RET(a_mgr.Open(1, true) != 0, 12);
    CHECK_OR_RET(a_mgr.Open(2, true) != 0, 13);
    CHECK_OR_RET(b_mgr.Open(1, false) != 0, 14);
    CHECK_OR_RET(b_mgr.Open(2, false) != 0, 15);

    for (int t = 0; t < 3000; ++t) {
        ++a_node.now_ms;
        ++b_node.now_ms;
        a_mgr.Tick();
        b_mgr.Tick();

        while (!a_node.out.empty()) {
            Packet p = a_node.out.back();
            a_node.out.pop_back();
            b_mgr.OnUdpPacket(p.key, &p.data[0], static_cast<uint16_t>(p.data.size()));
        }
        while (!b_node.out.empty()) {
            Packet p = b_node.out.back();
            b_node.out.pop_back();
            a_mgr.OnUdpPacket(p.key, &p.data[0], static_cast<uint16_t>(p.data.size()));
        }

        if (a_mgr.IsConnected(1) && a_mgr.IsConnected(2)) {
            break;
        }
    }
    CHECK_OR_RET(a_mgr.IsConnected(1), 20);
    CHECK_OR_RET(a_mgr.IsConnected(2), 21);
    CHECK_OR_RET(b_mgr.IsConnected(1), 22);
    CHECK_OR_RET(b_mgr.IsConnected(2), 23);
    CHECK_OR_RET(a_mgr.GetConnectionState(1) == rudp::ConnectionState::kConnected, 24);
    CHECK_OR_RET(a_mgr.GetConnectionState(2) == rudp::ConnectionState::kConnected, 25);

    rudp::Stats a_stats1;
    rudp::RuntimeMetrics a_metrics1;
    CHECK_OR_RET(a_mgr.GetStats(1, &a_stats1), 26);
    CHECK_OR_RET(a_mgr.GetRuntimeMetrics(1, &a_metrics1), 27);
    CHECK_OR_RET(a_stats1.connect_success > 0, 28);
    CHECK_OR_RET(a_metrics1.rto_ms >= cfg.retransmit_min_ms, 29);

    const char* m1 = "multi-conn-1";
    const char* m2 = "multi-conn-2";
    CHECK_OR_RET(a_mgr.Send(1, reinterpret_cast<const uint8_t*>(m1), static_cast<uint16_t>(strlen(m1) + 1)) == rudp::SendStatus::kOk, 30);
    CHECK_OR_RET(a_mgr.Send(2, reinterpret_cast<const uint8_t*>(m2), static_cast<uint16_t>(strlen(m2) + 1)) == rudp::SendStatus::kOk, 31);

    for (int t = 0; t < 2000; ++t) {
        ++a_node.now_ms;
        ++b_node.now_ms;
        a_mgr.Tick();
        b_mgr.Tick();

        while (!a_node.out.empty()) {
            Packet p = a_node.out.back();
            a_node.out.pop_back();
            b_mgr.OnUdpPacket(p.key, &p.data[0], static_cast<uint16_t>(p.data.size()));
        }
        while (!b_node.out.empty()) {
            Packet p = b_node.out.back();
            b_node.out.pop_back();
            a_mgr.OnUdpPacket(p.key, &p.data[0], static_cast<uint16_t>(p.data.size()));
        }
        if (b_node.delivered.size() >= 2) {
            break;
        }
    }

    bool got1 = false;
    bool got2 = false;
    for (size_t i = 0; i < b_node.delivered.size(); ++i) {
        const Delivered& d = b_node.delivered[i];
        if (d.key == 1 && strcmp(reinterpret_cast<const char*>(&d.data[0]), m1) == 0) {
            got1 = true;
        }
        if (d.key == 2 && strcmp(reinterpret_cast<const char*>(&d.data[0]), m2) == 0) {
            got2 = true;
        }
    }
    CHECK_OR_RET(got1 && got2, 40);

    rudp::Stats b_stats1;
    rudp::RuntimeMetrics b_metrics1;
    CHECK_OR_RET(b_mgr.GetStats(1, &b_stats1), 41);
    CHECK_OR_RET(b_mgr.GetRuntimeMetrics(1, &b_metrics1), 42);
    CHECK_OR_RET(b_stats1.rx_delivered > 0, 43);
    CHECK_OR_RET(b_metrics1.rto_ms >= cfg.retransmit_min_ms, 44);

    a_mgr.Remove(2);
    CHECK_OR_RET(!a_mgr.IsConnected(2), 45);
    CHECK_OR_RET(a_mgr.GetConnectionState(2) == rudp::ConnectionState::kDisconnected, 46);
    CHECK_OR_RET(!a_mgr.GetStats(2, &a_stats1), 47);
    CHECK_OR_RET(!a_mgr.GetRuntimeMetrics(2, &a_metrics1), 48);
    CHECK_OR_RET(a_mgr.GetActiveCount() == 1, 49);
    CHECK_OR_RET(!a_node.reentered_critical && !b_node.reentered_critical, 50);

    puts("rudp manager test passed");
    return 0;
}
