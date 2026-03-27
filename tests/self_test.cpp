#include "rudp/rudp.hpp"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>

struct Wire {
    uint32_t now_ms;
    bool block_a2b;
    bool block_b2a;
    uint32_t a2b_tx_count;
    uint32_t b2a_tx_count;
    std::vector<std::vector<uint8_t> > a2b;
    std::vector<std::vector<uint8_t> > b2a;
    std::vector<std::vector<uint8_t> > recv_a;
    std::vector<std::vector<uint8_t> > recv_b;
};

static uint32_t NowMs(void* u) {
    return static_cast<Wire*>(u)->now_ms;
}

static bool SendA(void* u, const uint8_t* d, uint16_t n) {
    Wire* w = static_cast<Wire*>(u);
    if (w->block_a2b) {
        return true;
    }
    std::vector<uint8_t> pkt(d, d + n);
    ++w->a2b_tx_count;
    w->a2b.push_back(pkt);
    return true;
}

static bool SendAVec(void* u, const uint8_t* h, uint16_t hn, const uint8_t* b, uint16_t bn) {
    std::vector<uint8_t> pkt;
    pkt.reserve(static_cast<size_t>(hn) + static_cast<size_t>(bn));
    pkt.insert(pkt.end(), h, h + hn);
    pkt.insert(pkt.end(), b, b + bn);
    return SendA(u, &pkt[0], static_cast<uint16_t>(pkt.size()));
}

static bool SendB(void* u, const uint8_t* d, uint16_t n) {
    Wire* w = static_cast<Wire*>(u);
    if (w->block_b2a) {
        return true;
    }
    std::vector<uint8_t> pkt(d, d + n);
    ++w->b2a_tx_count;
    w->b2a.push_back(pkt);
    return true;
}

static bool SendBVec(void* u, const uint8_t* h, uint16_t hn, const uint8_t* b, uint16_t bn) {
    std::vector<uint8_t> pkt;
    pkt.reserve(static_cast<size_t>(hn) + static_cast<size_t>(bn));
    pkt.insert(pkt.end(), h, h + hn);
    pkt.insert(pkt.end(), b, b + bn);
    return SendB(u, &pkt[0], static_cast<uint16_t>(pkt.size()));
}

static void DeliverA(void* u, const uint8_t* d, uint16_t n) {
    Wire* w = static_cast<Wire*>(u);
    w->recv_a.push_back(std::vector<uint8_t>(d, d + n));
}

static void DeliverB(void* u, const uint8_t* d, uint16_t n) {
    Wire* w = static_cast<Wire*>(u);
    w->recv_b.push_back(std::vector<uint8_t>(d, d + n));
}

int main() {
    Wire wire;
    wire.now_ms = 1;
    wire.block_a2b = false;
    wire.block_b2a = false;
    wire.a2b_tx_count = 0;
    wire.b2a_tx_count = 0;

    rudp::Endpoint a;
    rudp::Endpoint b;
    rudp::Config cfg = rudp::DefaultConfig();
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
    cfg.enable_auth = true;
    cfg.auth_psk = 0xA53C19D1u;
    cfg.auth_key0 = 0x0706050403020100ull;
    cfg.auth_key1 = 0x0F0E0D0C0B0A0908ull;

    rudp::Hooks ha = {&wire, NowMs, SendA, SendAVec, DeliverA, 0, 0};
    rudp::Hooks hb = {&wire, NowMs, SendB, SendBVec, DeliverB, 0, 0};
    assert(a.Init(cfg, ha));
    assert(b.Init(cfg, hb));
    assert(a.StartConnect());

    const auto PumpAll = [&]() {
        ++wire.now_ms;
        a.Tick();
        b.Tick();
        while (!wire.a2b.empty()) {
            std::vector<uint8_t> pkt = wire.a2b.back();
            wire.a2b.pop_back();
            b.OnUdpPacket(&pkt[0], static_cast<uint16_t>(pkt.size()));
        }
        while (!wire.b2a.empty()) {
            std::vector<uint8_t> pkt = wire.b2a.back();
            wire.b2a.pop_back();
            a.OnUdpPacket(&pkt[0], static_cast<uint16_t>(pkt.size()));
        }
    };
    const auto PumpOne = [&]() {
        ++wire.now_ms;
        a.Tick();
        b.Tick();
        if (!wire.a2b.empty()) {
            std::vector<uint8_t> pkt = wire.a2b.back();
            wire.a2b.pop_back();
            b.OnUdpPacket(&pkt[0], static_cast<uint16_t>(pkt.size()));
        }
        if (!wire.b2a.empty()) {
            std::vector<uint8_t> pkt = wire.b2a.back();
            wire.b2a.pop_back();
            a.OnUdpPacket(&pkt[0], static_cast<uint16_t>(pkt.size()));
        }
    };

    for (int t = 0; t < 2000; ++t) {
        PumpAll();
        if (a.IsConnected() && b.IsConnected()) {
            break;
        }
    }
    assert(a.IsConnected() && b.IsConnected());

    char msg[64];
    for (int i = 0; i < 50; ++i) {
        snprintf(msg, sizeof(msg), "hello-%d", i);
        bool queued = false;
        for (int spins = 0; spins < 4000; ++spins) {
            rudp::SendStatus st;
            if ((i % 3) == 0) {
                st = a.SendZeroCopy(reinterpret_cast<const uint8_t*>(msg), static_cast<uint16_t>(strlen(msg) + 1));
            } else {
                st = a.Send(reinterpret_cast<const uint8_t*>(msg), static_cast<uint16_t>(strlen(msg) + 1));
            }
            if (st == rudp::SendStatus::kOk) {
                queued = true;
                break;
            }
            assert(st == rudp::SendStatus::kQueueFull);
            PumpOne();
        }
        assert(queued);
    }

    for (int t = 0; t < 4000; ++t) {
        PumpOne();
        if (a.GetPendingSend() == 0 && wire.a2b.empty() && wire.b2a.empty()) {
            break;
        }
    }
    assert(a.IsConnected() && b.IsConnected());

    assert(a.SetAuthKey(2, 0x1112131415161718ull, 0x2122232425262728ull, false));
    assert(b.SetAuthKey(2, 0x1112131415161718ull, 0x2122232425262728ull, false));
    assert(a.ScheduleTxKeyRotation(2, 16));
    for (int i = 0; i < 20; ++i) {
        snprintf(msg, sizeof(msg), "rot-%d", i);
        bool queued = false;
        for (int spins = 0; spins < 4000; ++spins) {
            const rudp::SendStatus st =
                a.Send(reinterpret_cast<const uint8_t*>(msg), static_cast<uint16_t>(strlen(msg) + 1));
            if (st == rudp::SendStatus::kOk) {
                queued = true;
                break;
            }
            assert(st == rudp::SendStatus::kQueueFull);
            PumpOne();
        }
        assert(queued);
    }
    for (int t = 0; t < 4000; ++t) {
        PumpOne();
        if (a.GetPendingSend() == 0 && wire.a2b.empty() && wire.b2a.empty()) {
            break;
        }
    }

    rudp::Endpoint c;
    rudp::Hooks hc = {&wire, NowMs, SendA, SendAVec, DeliverA, 0, 0};
    assert(c.Init(cfg, hc));
    assert(c.StartConnect());
    wire.block_a2b = true;
    for (int t = 0; t < 1000; ++t) {
        ++wire.now_ms;
        c.Tick();
        if (c.GetConnectionState() == rudp::ConnectionState::kTimedOut) {
            break;
        }
    }
    assert(c.GetConnectionState() == rudp::ConnectionState::kTimedOut);

    puts("rudp self test passed");
    return 0;
}
