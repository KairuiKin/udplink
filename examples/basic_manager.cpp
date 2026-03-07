#include "rudp/manager.hpp"

#include <stdio.h>
#include <string.h>
#include <vector>

struct Pkt { uint64_t key; std::vector<uint8_t> data; };
struct Node {
    uint32_t now_ms;
    std::vector<Pkt> out;
};

static uint32_t NowMs(void* u) { return static_cast<Node*>(u)->now_ms; }
static bool SendRaw(void* u, uint64_t key, const uint8_t* d, uint16_t n) {
    Node* node = static_cast<Node*>(u);
    Pkt p;
    p.key = key;
    p.data.assign(d, d + n);
    node->out.push_back(p);
    return true;
}
static bool SendRawVec(void* u, uint64_t key, const uint8_t* h, uint16_t hn, const uint8_t* b, uint16_t bn) {
    std::vector<uint8_t> tmp;
    tmp.reserve(static_cast<size_t>(hn) + static_cast<size_t>(bn));
    tmp.insert(tmp.end(), h, h + hn);
    tmp.insert(tmp.end(), b, b + bn);
    return SendRaw(u, key, &tmp[0], static_cast<uint16_t>(tmp.size()));
}
static void OnDeliver(void*, uint64_t key, const uint8_t* d, uint16_t) {
    printf("deliver key=%llu msg=%s\n", static_cast<unsigned long long>(key), d);
}

int main() {
    Node a = {1}, b = {1};
    rudp::Config cfg = rudp::ConfigForProfile(rudp::ConfigProfile::kLossyLink);
    cfg.enable_auth = true;
    cfg.auth_key0 = 0x0706050403020100ull;
    cfg.auth_key1 = 0x0F0E0D0C0B0A0908ull;
    rudp::ManagerHooks ha = {&a, NowMs, SendRaw, SendRawVec, OnDeliver, 0, 0};
    rudp::ManagerHooks hb = {&b, NowMs, SendRaw, SendRawVec, OnDeliver, 0, 0};
    rudp::ConnectionManager ma, mb;
    if (!ma.Init(cfg, ha) || !mb.Init(cfg, hb)) return 1;
    ma.Open(1001, true);
    ma.Open(2002, true);
    mb.Open(1001, false);
    mb.Open(2002, false);

    for (int i = 0; i < 3000; ++i) {
        ++a.now_ms; ++b.now_ms;
        ma.Tick(); mb.Tick();
        while (!a.out.empty()) {
            Pkt p = a.out.back(); a.out.pop_back();
            mb.OnUdpPacket(p.key, &p.data[0], static_cast<uint16_t>(p.data.size()));
        }
        while (!b.out.empty()) {
            Pkt p = b.out.back(); b.out.pop_back();
            ma.OnUdpPacket(p.key, &p.data[0], static_cast<uint16_t>(p.data.size()));
        }
    }

    const char* m1 = "m1";
    const char* m2 = "m2";
    ma.Send(1001, reinterpret_cast<const uint8_t*>(m1), static_cast<uint16_t>(strlen(m1) + 1));
    ma.Send(2002, reinterpret_cast<const uint8_t*>(m2), static_cast<uint16_t>(strlen(m2) + 1));
    for (int i = 0; i < 800; ++i) {
        ++a.now_ms; ++b.now_ms;
        ma.Tick(); mb.Tick();
        while (!a.out.empty()) {
            Pkt p = a.out.back(); a.out.pop_back();
            mb.OnUdpPacket(p.key, &p.data[0], static_cast<uint16_t>(p.data.size()));
        }
        while (!b.out.empty()) {
            Pkt p = b.out.back(); b.out.pop_back();
            ma.OnUdpPacket(p.key, &p.data[0], static_cast<uint16_t>(p.data.size()));
        }
    }
    return 0;
}
