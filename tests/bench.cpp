#include "rudp/rudp.hpp"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <vector>

struct BenchWire {
    uint32_t now_ms;
    std::vector<std::vector<uint8_t> > a2b;
    std::vector<std::vector<uint8_t> > b2a;
    uint32_t delivered;
};

static uint32_t BNow(void* u) {
    return static_cast<BenchWire*>(u)->now_ms;
}

static bool BSendA(void* u, const uint8_t* d, uint16_t n) {
    BenchWire* w = static_cast<BenchWire*>(u);
    w->a2b.push_back(std::vector<uint8_t>(d, d + n));
    return true;
}

static bool BSendB(void* u, const uint8_t* d, uint16_t n) {
    BenchWire* w = static_cast<BenchWire*>(u);
    w->b2a.push_back(std::vector<uint8_t>(d, d + n));
    return true;
}

static bool BSendAVec(void* u, const uint8_t* h, uint16_t hn, const uint8_t* b, uint16_t bn) {
    std::vector<uint8_t> pkt;
    pkt.reserve(static_cast<size_t>(hn) + static_cast<size_t>(bn));
    pkt.insert(pkt.end(), h, h + hn);
    pkt.insert(pkt.end(), b, b + bn);
    return BSendA(u, &pkt[0], static_cast<uint16_t>(pkt.size()));
}

static bool BSendBVec(void* u, const uint8_t* h, uint16_t hn, const uint8_t* b, uint16_t bn) {
    std::vector<uint8_t> pkt;
    pkt.reserve(static_cast<size_t>(hn) + static_cast<size_t>(bn));
    pkt.insert(pkt.end(), h, h + hn);
    pkt.insert(pkt.end(), b, b + bn);
    return BSendB(u, &pkt[0], static_cast<uint16_t>(pkt.size()));
}

static void BDeliverA(void*, const uint8_t*, uint16_t) {}

static void BDeliverB(void* u, const uint8_t*, uint16_t) {
    BenchWire* w = static_cast<BenchWire*>(u);
    ++w->delivered;
}

int main() {
    const uint32_t kMessages = 20000;
    const uint16_t kPayloadSize = 96;

    BenchWire wire;
    wire.now_ms = 1;
    wire.delivered = 0;

    rudp::Config cfg = rudp::DefaultConfig();
    cfg.mtu = 192;
    cfg.max_payload = 120;
    cfg.send_window = 32;
    cfg.recv_window = 32;
    cfg.retransmit_min_ms = 10;
    cfg.retransmit_max_ms = 200;
    cfg.connect_retry_ms = 20;
    cfg.max_connect_retries = 16;
    cfg.enable_auth = true;
    cfg.auth_psk = 0xA53C19D1u;
    cfg.auth_key0 = 0x0706050403020100ull;
    cfg.auth_key1 = 0x0F0E0D0C0B0A0908ull;

    rudp::Endpoint a;
    rudp::Endpoint b;
    rudp::Hooks ha = {&wire, BNow, BSendA, BSendAVec, BDeliverA};
    rudp::Hooks hb = {&wire, BNow, BSendB, BSendBVec, BDeliverB};
    if (!a.Init(cfg, ha) || !b.Init(cfg, hb)) {
        puts("bench init failed");
        return 1;
    }

    a.StartConnect();
    for (int i = 0; i < 3000; ++i) {
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
        if (a.IsConnected() && b.IsConnected()) {
            break;
        }
    }
    if (!a.IsConnected() || !b.IsConnected()) {
        puts("bench connect failed");
        return 2;
    }

    uint8_t payload[kPayloadSize];
    memset(payload, 'x', sizeof(payload));

    const clock_t t0 = clock();
    uint32_t sent = 0;
    bool rotated = false;
    while (wire.delivered < kMessages) {
        ++wire.now_ms;
        a.Tick();
        b.Tick();

        if (!rotated && sent >= (kMessages / 2u)) {
            a.SetAuthKey(2, 0x3132333435363738ull, 0x4142434445464748ull, false);
            b.SetAuthKey(2, 0x3132333435363738ull, 0x4142434445464748ull, false);
            a.ScheduleTxKeyRotation(2, 32);
            b.ScheduleTxKeyRotation(2, 32);
            rotated = true;
        }

        while (sent < kMessages) {
            rudp::SendStatus st = ((sent & 1u) == 0u)
                                      ? a.Send(payload, kPayloadSize)
                                      : a.SendZeroCopy(payload, kPayloadSize);
            if (st != rudp::SendStatus::kOk) {
                break;
            }
            ++sent;
        }

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
    }
    const clock_t t1 = clock();

    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    const double msgps = static_cast<double>(kMessages) / sec;
    const double mbps = (static_cast<double>(kMessages) * static_cast<double>(kPayloadSize)) / (1024.0 * 1024.0) / sec;

    printf("rudp bench: messages=%u payload=%uB time=%.3fs msg/s=%.1f MB/s=%.2f\n",
           static_cast<unsigned>(kMessages),
           static_cast<unsigned>(kPayloadSize),
           sec, msgps, mbps);
    return 0;
}
