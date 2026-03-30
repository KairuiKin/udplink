#include "rudp/rudp.hpp"

#include <stdio.h>
#include <stdlib.h>
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

static bool EnqueuePacket(std::vector<std::vector<uint8_t> >* queue, const uint8_t* d, uint16_t n) {
    if (!queue || (!d && n > 0)) {
        return false;
    }
    queue->push_back(std::vector<uint8_t>(d, d + n));
    return true;
}

static bool EnqueuePacketVec(std::vector<std::vector<uint8_t> >* queue,
                             const uint8_t* h, uint16_t hn,
                             const uint8_t* b, uint16_t bn) {
    if (!queue || (!h && hn > 0) || (!b && bn > 0)) {
        return false;
    }
    std::vector<uint8_t> pkt;
    pkt.reserve(static_cast<size_t>(hn) + static_cast<size_t>(bn));
    pkt.insert(pkt.end(), h, h + hn);
    pkt.insert(pkt.end(), b, b + bn);
    queue->push_back(std::move(pkt));
    return true;
}

static bool BSendA(void* u, const uint8_t* d, uint16_t n) {
    BenchWire* w = static_cast<BenchWire*>(u);
    return EnqueuePacket(&w->a2b, d, n);
}

static bool BSendB(void* u, const uint8_t* d, uint16_t n) {
    BenchWire* w = static_cast<BenchWire*>(u);
    return EnqueuePacket(&w->b2a, d, n);
}

static bool BSendAVec(void* u, const uint8_t* h, uint16_t hn, const uint8_t* b, uint16_t bn) {
    BenchWire* w = static_cast<BenchWire*>(u);
    return EnqueuePacketVec(&w->a2b, h, hn, b, bn);
}

static bool BSendBVec(void* u, const uint8_t* h, uint16_t hn, const uint8_t* b, uint16_t bn) {
    BenchWire* w = static_cast<BenchWire*>(u);
    return EnqueuePacketVec(&w->b2a, h, hn, b, bn);
}

static void BDeliverA(void*, const uint8_t*, uint16_t) {}

static void BDeliverB(void* u, const uint8_t*, uint16_t) {
    BenchWire* w = static_cast<BenchWire*>(u);
    ++w->delivered;
}

static bool ParseU32Arg(const char* text, uint32_t* out) {
    if (!text || !out || *text == '\0') {
        return false;
    }
    char* end = 0;
    const unsigned long value = strtoul(text, &end, 10);
    if (*end != '\0') {
        return false;
    }
    *out = static_cast<uint32_t>(value);
    return true;
}

int main(int argc, char** argv) {
    uint32_t messages = 20000;
    uint32_t payload_size_u32 = 96;
    if (argc >= 2 && !ParseU32Arg(argv[1], &messages)) {
        fprintf(stderr, "usage: %s [messages] [payload_bytes]\n", argv[0]);
        return 3;
    }
    if (argc >= 3 && !ParseU32Arg(argv[2], &payload_size_u32)) {
        fprintf(stderr, "usage: %s [messages] [payload_bytes]\n", argv[0]);
        return 3;
    }
    if (messages == 0) {
        fputs("bench messages must be > 0\n", stderr);
        return 3;
    }
    if (payload_size_u32 == 0 || payload_size_u32 > 120u) {
        fputs("bench payload must be in range 1..120 bytes\n", stderr);
        return 3;
    }
    const uint16_t payload_size = static_cast<uint16_t>(payload_size_u32);

    BenchWire wire;
    wire.now_ms = 1;
    wire.delivered = 0;

    wire.a2b.reserve(64);
    wire.b2a.reserve(64);

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
    rudp::Hooks ha = {&wire, BNow, BSendA, BSendAVec, BDeliverA, 0, 0};
    rudp::Hooks hb = {&wire, BNow, BSendB, BSendBVec, BDeliverB, 0, 0};
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
            std::vector<uint8_t> pkt = std::move(wire.a2b.back());
            wire.a2b.pop_back();
            b.OnUdpPacket(&pkt[0], static_cast<uint16_t>(pkt.size()));
        }
        while (!wire.b2a.empty()) {
            std::vector<uint8_t> pkt = std::move(wire.b2a.back());
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

    std::vector<uint8_t> payload(payload_size, static_cast<uint8_t>('x'));

    const clock_t t0 = clock();
    uint32_t sent = 0;
    bool rotated = false;
    while (wire.delivered < messages) {
        ++wire.now_ms;
        a.Tick();
        b.Tick();

        if (!rotated && sent >= (messages / 2u)) {
            a.SetAuthKey(2, 0x3132333435363738ull, 0x4142434445464748ull, false);
            b.SetAuthKey(2, 0x3132333435363738ull, 0x4142434445464748ull, false);
            a.ScheduleTxKeyRotation(2, 32);
            b.ScheduleTxKeyRotation(2, 32);
            rotated = true;
        }

        while (sent < messages) {
            rudp::SendStatus st = ((sent & 1u) == 0u)
                                      ? a.Send(payload.data(), payload_size)
                                      : a.SendZeroCopy(payload.data(), payload_size);
            if (st != rudp::SendStatus::kOk) {
                break;
            }
            ++sent;
        }

        while (!wire.a2b.empty()) {
            std::vector<uint8_t> pkt = std::move(wire.a2b.back());
            wire.a2b.pop_back();
            b.OnUdpPacket(&pkt[0], static_cast<uint16_t>(pkt.size()));
        }
        while (!wire.b2a.empty()) {
            std::vector<uint8_t> pkt = std::move(wire.b2a.back());
            wire.b2a.pop_back();
            a.OnUdpPacket(&pkt[0], static_cast<uint16_t>(pkt.size()));
        }
    }
    const clock_t t1 = clock();

    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    const double msgps = static_cast<double>(messages) / sec;
    const double mbps = (static_cast<double>(messages) * static_cast<double>(payload_size)) / (1024.0 * 1024.0) / sec;

    printf("rudp bench: messages=%u payload=%uB time=%.3fs msg/s=%.1f MB/s=%.2f\n",
           static_cast<unsigned>(messages),
           static_cast<unsigned>(payload_size),
           sec, msgps, mbps);
    return 0;
}
