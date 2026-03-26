#include "rudp/rudp.hpp"

#include <stdio.h>
#include <string.h>
#include <vector>

struct Wire {
    uint32_t now_ms;
    std::vector<std::vector<uint8_t> > a2b;
    std::vector<std::vector<uint8_t> > b2a;
};

static uint32_t NowMs(void* u) { return static_cast<Wire*>(u)->now_ms; }

static bool SendA(void* u, const uint8_t* d, uint16_t n) {
    Wire* w = static_cast<Wire*>(u);
    w->a2b.push_back(std::vector<uint8_t>(d, d + n));
    return true;
}

static bool SendB(void* u, const uint8_t* d, uint16_t n) {
    Wire* w = static_cast<Wire*>(u);
    w->b2a.push_back(std::vector<uint8_t>(d, d + n));
    return true;
}

static void DeliverA(void*, const uint8_t* d, uint16_t) { printf("A recv: %s\n", d); }
static void DeliverB(void*, const uint8_t* d, uint16_t) { printf("B recv: %s\n", d); }

int main() {
    Wire wire;
    wire.now_ms = 1;

    // DOCS_SNIPPET_BEGIN:basic_endpoint
    rudp::Config cfg = rudp::ConfigForProfile(rudp::ConfigProfile::kBalanced);
    cfg.enable_auth = true;
    cfg.auth_key0 = 0x0706050403020100ull;
    cfg.auth_key1 = 0x0F0E0D0C0B0A0908ull;

    rudp::Endpoint a;
    rudp::Endpoint b;
    rudp::Hooks ha = {&wire, NowMs, SendA, 0, DeliverA, 0, 0};
    rudp::Hooks hb = {&wire, NowMs, SendB, 0, DeliverB, 0, 0};
    if (!a.Init(cfg, ha) || !b.Init(cfg, hb)) return 1;
    a.StartConnect();
    // DOCS_SNIPPET_END:basic_endpoint

    for (int i = 0; i < 2000; ++i) {
        ++wire.now_ms;
        a.Tick();
        b.Tick();
        while (!wire.a2b.empty()) {
            std::vector<uint8_t> p = wire.a2b.back();
            wire.a2b.pop_back();
            b.OnUdpPacket(&p[0], static_cast<uint16_t>(p.size()));
        }
        while (!wire.b2a.empty()) {
            std::vector<uint8_t> p = wire.b2a.back();
            wire.b2a.pop_back();
            a.OnUdpPacket(&p[0], static_cast<uint16_t>(p.size()));
        }
        if (a.IsConnected() && b.IsConnected()) break;
    }

    const char* msg = "hello-from-a";
    a.Send(reinterpret_cast<const uint8_t*>(msg), static_cast<uint16_t>(strlen(msg) + 1));

    for (int i = 0; i < 300; ++i) {
        ++wire.now_ms;
        a.Tick();
        b.Tick();
        while (!wire.a2b.empty()) {
            std::vector<uint8_t> p = wire.a2b.back();
            wire.a2b.pop_back();
            b.OnUdpPacket(&p[0], static_cast<uint16_t>(p.size()));
        }
        while (!wire.b2a.empty()) {
            std::vector<uint8_t> p = wire.b2a.back();
            wire.b2a.pop_back();
            a.OnUdpPacket(&p[0], static_cast<uint16_t>(p.size()));
        }
    }

    rudp::RuntimeMetrics m = a.GetRuntimeMetrics();
    printf("A metrics: pending=%u rto=%u rtt=%u rtx_permille=%u drop_permille=%u\n",
           static_cast<unsigned>(m.pending_send),
           static_cast<unsigned>(m.rto_ms),
           static_cast<unsigned>(m.smoothed_rtt_ms),
           static_cast<unsigned>(m.tx_retransmit_ratio_permille),
           static_cast<unsigned>(m.rx_drop_ratio_permille));
    return 0;
}
