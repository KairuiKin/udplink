#include "rudp/rudp.hpp"

#ifndef _WIN32

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

struct PeerCtx {
    int sock;
    sockaddr_in peer_addr;
    uint32_t now_ms;
    const char* tag;
};

static uint32_t NowMs(void* u) {
    return static_cast<PeerCtx*>(u)->now_ms;
}

static bool SendRaw(void* u, const uint8_t* data, uint16_t len) {
    PeerCtx* ctx = static_cast<PeerCtx*>(u);
    const ssize_t n = sendto(ctx->sock, data, len, 0,
                             reinterpret_cast<const sockaddr*>(&ctx->peer_addr),
                             sizeof(ctx->peer_addr));
    return n == static_cast<ssize_t>(len);
}

static void OnDeliver(void* u, const uint8_t* data, uint16_t) {
    PeerCtx* ctx = static_cast<PeerCtx*>(u);
    printf("[%s] recv: %s\n", ctx->tag, data);
}

static bool MakeNonBlocking(int s) {
    const int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) return false;
    return fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
}

int main() {
    int sa = socket(AF_INET, SOCK_DGRAM, 0);
    int sb = socket(AF_INET, SOCK_DGRAM, 0);
    if (sa < 0 || sb < 0) return 1;

    sockaddr_in a_addr;
    sockaddr_in b_addr;
    memset(&a_addr, 0, sizeof(a_addr));
    memset(&b_addr, 0, sizeof(b_addr));
    a_addr.sin_family = AF_INET;
    b_addr.sin_family = AF_INET;
    a_addr.sin_port = htons(29001);
    b_addr.sin_port = htons(29002);
    a_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    b_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(sa, reinterpret_cast<sockaddr*>(&a_addr), sizeof(a_addr)) != 0) return 2;
    if (bind(sb, reinterpret_cast<sockaddr*>(&b_addr), sizeof(b_addr)) != 0) return 3;
    if (!MakeNonBlocking(sa) || !MakeNonBlocking(sb)) return 4;

    PeerCtx ca = {sa, b_addr, 1, "A"};
    PeerCtx cb = {sb, a_addr, 1, "B"};

    rudp::Config cfg = rudp::ConfigForProfile(rudp::ConfigProfile::kBalanced);
    cfg.enable_auth = true;
    cfg.auth_key0 = 0x0706050403020100ull;
    cfg.auth_key1 = 0x0F0E0D0C0B0A0908ull;

    rudp::Endpoint a;
    rudp::Endpoint b;
    rudp::Hooks ha = {&ca, NowMs, SendRaw, 0, OnDeliver, 0, 0};
    rudp::Hooks hb = {&cb, NowMs, SendRaw, 0, OnDeliver, 0, 0};
    if (!a.Init(cfg, ha) || !b.Init(cfg, hb)) return 5;
    a.StartConnect();

    uint8_t buf[2048];
    for (int i = 0; i < 3000; ++i) {
        ++ca.now_ms;
        ++cb.now_ms;
        a.Tick();
        b.Tick();

        for (;;) {
            ssize_t n = recvfrom(sa, buf, sizeof(buf), 0, 0, 0);
            if (n <= 0) break;
            a.OnUdpPacket(buf, static_cast<uint16_t>(n));
        }
        for (;;) {
            ssize_t n = recvfrom(sb, buf, sizeof(buf), 0, 0, 0);
            if (n <= 0) break;
            b.OnUdpPacket(buf, static_cast<uint16_t>(n));
        }
        if (a.IsConnected() && b.IsConnected()) break;
    }

    const char* msg = "hello-posix-udp";
    a.Send(reinterpret_cast<const uint8_t*>(msg), static_cast<uint16_t>(strlen(msg) + 1));
    for (int i = 0; i < 800; ++i) {
        ++ca.now_ms;
        ++cb.now_ms;
        a.Tick();
        b.Tick();
        for (;;) {
            ssize_t n = recvfrom(sa, buf, sizeof(buf), 0, 0, 0);
            if (n <= 0) break;
            a.OnUdpPacket(buf, static_cast<uint16_t>(n));
        }
        for (;;) {
            ssize_t n = recvfrom(sb, buf, sizeof(buf), 0, 0, 0);
            if (n <= 0) break;
            b.OnUdpPacket(buf, static_cast<uint16_t>(n));
        }
    }

    close(sa);
    close(sb);
    return 0;
}

#else
int main() { return 0; }
#endif
