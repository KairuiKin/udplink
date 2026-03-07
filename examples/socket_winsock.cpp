#include "rudp/rudp.hpp"

#ifdef _WIN32

#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

struct PeerCtx {
    SOCKET sock;
    sockaddr_in peer_addr;
    uint32_t now_ms;
    const char* tag;
};

static uint32_t NowMs(void* u) {
    return static_cast<PeerCtx*>(u)->now_ms;
}

static bool SendRaw(void* u, const uint8_t* data, uint16_t len) {
    PeerCtx* ctx = static_cast<PeerCtx*>(u);
    const int n = sendto(ctx->sock, reinterpret_cast<const char*>(data), len, 0,
                         reinterpret_cast<const sockaddr*>(&ctx->peer_addr),
                         static_cast<int>(sizeof(ctx->peer_addr)));
    return n == static_cast<int>(len);
}

static void OnDeliver(void* u, const uint8_t* data, uint16_t) {
    PeerCtx* ctx = static_cast<PeerCtx*>(u);
    printf("[%s] recv: %s\n", ctx->tag, data);
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    SOCKET sa = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    SOCKET sb = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sa == INVALID_SOCKET || sb == INVALID_SOCKET) return 2;

    sockaddr_in a_addr;
    sockaddr_in b_addr;
    memset(&a_addr, 0, sizeof(a_addr));
    memset(&b_addr, 0, sizeof(b_addr));
    a_addr.sin_family = AF_INET;
    b_addr.sin_family = AF_INET;
    a_addr.sin_port = htons(29101);
    b_addr.sin_port = htons(29102);
    a_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    b_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(sa, reinterpret_cast<sockaddr*>(&a_addr), sizeof(a_addr)) != 0) return 3;
    if (bind(sb, reinterpret_cast<sockaddr*>(&b_addr), sizeof(b_addr)) != 0) return 4;

    u_long nonblock = 1;
    ioctlsocket(sa, FIONBIO, &nonblock);
    ioctlsocket(sb, FIONBIO, &nonblock);

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
            int from_len = sizeof(sockaddr_in);
            int n = recvfrom(sa, reinterpret_cast<char*>(buf), sizeof(buf), 0, 0, &from_len);
            if (n <= 0) break;
            a.OnUdpPacket(buf, static_cast<uint16_t>(n));
        }
        for (;;) {
            int from_len = sizeof(sockaddr_in);
            int n = recvfrom(sb, reinterpret_cast<char*>(buf), sizeof(buf), 0, 0, &from_len);
            if (n <= 0) break;
            b.OnUdpPacket(buf, static_cast<uint16_t>(n));
        }
        if (a.IsConnected() && b.IsConnected()) break;
    }

    const char* msg = "hello-winsock-udp";
    a.Send(reinterpret_cast<const uint8_t*>(msg), static_cast<uint16_t>(strlen(msg) + 1));
    for (int i = 0; i < 800; ++i) {
        ++ca.now_ms;
        ++cb.now_ms;
        a.Tick();
        b.Tick();
        for (;;) {
            int from_len = sizeof(sockaddr_in);
            int n = recvfrom(sa, reinterpret_cast<char*>(buf), sizeof(buf), 0, 0, &from_len);
            if (n <= 0) break;
            a.OnUdpPacket(buf, static_cast<uint16_t>(n));
        }
        for (;;) {
            int from_len = sizeof(sockaddr_in);
            int n = recvfrom(sb, reinterpret_cast<char*>(buf), sizeof(buf), 0, 0, &from_len);
            if (n <= 0) break;
            b.OnUdpPacket(buf, static_cast<uint16_t>(n));
        }
    }

    closesocket(sa);
    closesocket(sb);
    WSACleanup();
    return 0;
}

#else
int main() { return 0; }
#endif
