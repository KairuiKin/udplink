#include "rudp/rudp.hpp"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <string>
#include <vector>

struct ScheduledPacket {
    uint32_t due_ms;
    std::vector<uint8_t> bytes;
};

struct Wire {
    uint32_t now_ms;
    bool impair_a2b;
    uint32_t a2b_tx_count;
    uint32_t b2a_tx_count;
    std::vector<ScheduledPacket> a2b;
    std::vector<ScheduledPacket> b2a;
    std::vector<std::string> recv_b;
};

static bool ShouldDropA2B(uint32_t tx_count) {
    return tx_count == 5u || tx_count == 11u || tx_count == 17u || tx_count == 23u;
}

static uint32_t DelayA2B(uint32_t tx_count) {
    if (tx_count == 2u || tx_count == 8u || tx_count == 14u || tx_count == 20u) {
        return 7u;
    }
    if (tx_count == 3u || tx_count == 9u || tx_count == 15u || tx_count == 21u) {
        return 3u;
    }
    return 1u;
}

static uint32_t NowMs(void* u) {
    return static_cast<Wire*>(u)->now_ms;
}

static bool QueuePacket(std::vector<ScheduledPacket>* queue,
                        uint32_t due_ms,
                        const uint8_t* data,
                        uint16_t len) {
    ScheduledPacket pkt;
    pkt.due_ms = due_ms;
    pkt.bytes.assign(data, data + len);
    queue->push_back(pkt);
    return true;
}

static bool SendA(void* u, const uint8_t* data, uint16_t len) {
    Wire* wire = static_cast<Wire*>(u);
    ++wire->a2b_tx_count;
    if (wire->impair_a2b && ShouldDropA2B(wire->a2b_tx_count)) {
        return true;
    }
    const uint32_t delay = wire->impair_a2b ? DelayA2B(wire->a2b_tx_count) : 1u;
    return QueuePacket(&wire->a2b, wire->now_ms + delay, data, len);
}

static bool SendAVec(void* u,
                     const uint8_t* head, uint16_t head_len,
                     const uint8_t* body, uint16_t body_len) {
    std::vector<uint8_t> pkt;
    pkt.reserve(static_cast<size_t>(head_len) + static_cast<size_t>(body_len));
    pkt.insert(pkt.end(), head, head + head_len);
    pkt.insert(pkt.end(), body, body + body_len);
    return SendA(u, &pkt[0], static_cast<uint16_t>(pkt.size()));
}

static bool SendB(void* u, const uint8_t* data, uint16_t len) {
    Wire* wire = static_cast<Wire*>(u);
    ++wire->b2a_tx_count;
    return QueuePacket(&wire->b2a, wire->now_ms + 1u, data, len);
}

static bool SendBVec(void* u,
                     const uint8_t* head, uint16_t head_len,
                     const uint8_t* body, uint16_t body_len) {
    std::vector<uint8_t> pkt;
    pkt.reserve(static_cast<size_t>(head_len) + static_cast<size_t>(body_len));
    pkt.insert(pkt.end(), head, head + head_len);
    pkt.insert(pkt.end(), body, body + body_len);
    return SendB(u, &pkt[0], static_cast<uint16_t>(pkt.size()));
}

static void DeliverB(void* u, const uint8_t* data, uint16_t len) {
    Wire* wire = static_cast<Wire*>(u);
    wire->recv_b.push_back(std::string(reinterpret_cast<const char*>(data),
                                       reinterpret_cast<const char*>(data) + len));
}

static void DeliverA(void*, const uint8_t*, uint16_t) {}

static void PumpDue(std::vector<ScheduledPacket>* queue, rudp::Endpoint* dst, uint32_t now_ms) {
    for (size_t i = 0; i < queue->size();) {
        if ((*queue)[i].due_ms > now_ms) {
            ++i;
            continue;
        }
        const ScheduledPacket pkt = (*queue)[i];
        queue->erase(queue->begin() + i);
        dst->OnUdpPacket(&pkt.bytes[0], static_cast<uint16_t>(pkt.bytes.size()));
    }
}

int main() {
    Wire wire;
    wire.now_ms = 1;
    wire.impair_a2b = false;
    wire.a2b_tx_count = 0;
    wire.b2a_tx_count = 0;

    rudp::Config cfg = rudp::ConfigForProfile(rudp::ConfigProfile::kLossyLink);
    cfg.mtu = 160;
    cfg.max_payload = 96;
    cfg.send_window = 16;
    cfg.recv_window = 16;
    cfg.retransmit_min_ms = 8;
    cfg.retransmit_max_ms = 160;
    cfg.connect_retry_ms = 20;
    cfg.max_connect_retries = 8;
    cfg.idle_timeout_ms = 2000;
    cfg.heartbeat_ms = 80;
    cfg.enable_auth = true;
    cfg.auth_key0 = 0x0706050403020100ull;
    cfg.auth_key1 = 0x0F0E0D0C0B0A0908ull;

    rudp::Endpoint a;
    rudp::Endpoint b;
    rudp::Hooks ha = {&wire, NowMs, SendA, SendAVec, DeliverA, 0, 0};
    rudp::Hooks hb = {&wire, NowMs, SendB, SendBVec, DeliverB, 0, 0};
    assert(a.Init(cfg, ha));
    assert(b.Init(cfg, hb));
    assert(a.StartConnect());

    for (int i = 0; i < 2000; ++i) {
        ++wire.now_ms;
        a.Tick();
        b.Tick();
        PumpDue(&wire.a2b, &b, wire.now_ms);
        PumpDue(&wire.b2a, &a, wire.now_ms);
        if (a.IsConnected() && b.IsConnected()) {
            break;
        }
    }
    assert(a.IsConnected() && b.IsConnected());

    wire.impair_a2b = true;

    static const int kMessageCount = 24;
    char msg[32];
    for (int i = 0; i < kMessageCount; ++i) {
        snprintf(msg, sizeof(msg), "msg-%02d", i);
        bool queued = false;
        for (int spins = 0; spins < 4000; ++spins) {
            if (a.Send(reinterpret_cast<const uint8_t*>(msg),
                       static_cast<uint16_t>(strlen(msg) + 1)) == rudp::SendStatus::kOk) {
                queued = true;
                break;
            }
            ++wire.now_ms;
            a.Tick();
            b.Tick();
            PumpDue(&wire.a2b, &b, wire.now_ms);
            PumpDue(&wire.b2a, &a, wire.now_ms);
        }
        assert(queued);
    }

    for (int i = 0; i < 20000; ++i) {
        ++wire.now_ms;
        a.Tick();
        b.Tick();
        PumpDue(&wire.a2b, &b, wire.now_ms);
        PumpDue(&wire.b2a, &a, wire.now_ms);
        if (wire.recv_b.size() == static_cast<size_t>(kMessageCount) &&
            a.GetPendingSend() == 0) {
            break;
        }
    }

    assert(wire.recv_b.size() == static_cast<size_t>(kMessageCount));
    for (int i = 0; i < kMessageCount; ++i) {
        snprintf(msg, sizeof(msg), "msg-%02d", i);
        assert(wire.recv_b[static_cast<size_t>(i)] == std::string(msg, strlen(msg) + 1));
    }

    const rudp::Stats& stats = a.GetStats();
    assert(stats.tx_retransmits > 0);
    assert(stats.rx_duplicates > 0 || stats.tx_packets > static_cast<uint32_t>(kMessageCount));

    puts("rudp reliability test passed");
    return 0;
}
