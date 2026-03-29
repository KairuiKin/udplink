#include "rudp/rudp.hpp"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_compat;
static void CloseSock(SOCKET s) { closesocket(s); }
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int SOCKET;
typedef int socklen_compat;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
static void CloseSock(int s) { close(s); }
#endif

struct PeerCtx {
    SOCKET sock;
    sockaddr_in peer_addr;
    uint32_t now_ms;
    uint32_t tx_count;
    uint32_t rx_count;
};

static uint32_t NowMs(void* user) {
    return static_cast<PeerCtx*>(user)->now_ms;
}

static bool SendRaw(void* user, const uint8_t* data, uint16_t len) {
    PeerCtx* ctx = static_cast<PeerCtx*>(user);
    const int n = sendto(ctx->sock,
#ifdef _WIN32
                         reinterpret_cast<const char*>(data),
#else
                         data,
#endif
                         len,
                         0,
                         reinterpret_cast<const sockaddr*>(&ctx->peer_addr),
                         static_cast<socklen_compat>(sizeof(ctx->peer_addr)));
    return n == static_cast<int>(len);
}

static void DumpHex(const uint8_t* data, uint16_t len) {
    for (uint16_t i = 0; i < len; ++i) {
        printf("%02X", static_cast<unsigned>(data[i]));
        if (i + 1 != len) {
            putchar(' ');
        }
    }
}

static void OnDeliver(void* user, const uint8_t* data, uint16_t len) {
    PeerCtx* ctx = static_cast<PeerCtx*>(user);
    ++ctx->rx_count;
    printf("[peer] delivered packet #%u len=%u data=", static_cast<unsigned>(ctx->rx_count), static_cast<unsigned>(len));
    DumpHex(data, len);
    printf("\n");
}

static bool MakeNonBlocking(SOCKET s) {
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(s, FIONBIO, &mode) == 0;
#else
    const int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    return fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

static bool ParseIpv4(const char* text, in_addr* out) {
#ifdef _WIN32
    return InetPtonA(AF_INET, text, out) == 1;
#else
    return inet_pton(AF_INET, text, out) == 1;
#endif
}

int main(int argc, char** argv) {
    if (argc != 5) {
        fprintf(stderr, "usage: %s <local-ip> <local-port> <peer-ip> <peer-port>\n", argv[0]);
        return 1;
    }

    const char* local_ip = argv[1];
    const uint16_t local_port = static_cast<uint16_t>(atoi(argv[2]));
    const char* peer_ip = argv[3];
    const uint16_t peer_port = static_cast<uint16_t>(atoi(argv[4]));

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 2;
    }
#endif

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "socket creation failed\n");
#ifdef _WIN32
        WSACleanup();
#endif
        return 3;
    }

    sockaddr_in local_addr;
    sockaddr_in remote_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    memset(&remote_addr, 0, sizeof(remote_addr));
    local_addr.sin_family = AF_INET;
    remote_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(local_port);
    remote_addr.sin_port = htons(peer_port);

    if (!ParseIpv4(local_ip, &local_addr.sin_addr) || !ParseIpv4(peer_ip, &remote_addr.sin_addr)) {
        fprintf(stderr, "invalid IPv4 address\n");
        CloseSock(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 4;
    }

    if (bind(sock, reinterpret_cast<sockaddr*>(&local_addr), sizeof(local_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind failed\n");
        CloseSock(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 5;
    }

    if (!MakeNonBlocking(sock)) {
        fprintf(stderr, "failed to make socket non-blocking\n");
        CloseSock(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 6;
    }

    PeerCtx ctx;
    ctx.sock = sock;
    ctx.peer_addr = remote_addr;
    ctx.now_ms = 1;
    ctx.tx_count = 0;
    ctx.rx_count = 0;

    rudp::Config cfg = rudp::ConfigForProfile(rudp::ConfigProfile::kLowPower);
    cfg.max_payload = 96;
    cfg.enable_auth = true;
    cfg.auth_key0 = 0x0123456789ABCDEFull;
    cfg.auth_key1 = 0xFEDCBA9876543210ull;

    rudp::Endpoint endpoint;
    rudp::Hooks hooks = {&ctx, NowMs, SendRaw, 0, OnDeliver, 0, 0};
    if (!endpoint.Init(cfg, hooks)) {
        fprintf(stderr, "endpoint init failed\n");
        CloseSock(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 7;
    }

    printf("[peer] waiting on %s:%u for board %s:%u\n",
           local_ip,
           static_cast<unsigned>(local_port),
           peer_ip,
           static_cast<unsigned>(peer_port));

    rudp::ConnectionState last_state = endpoint.GetConnectionState();
    uint32_t last_tx_ms = 0;
    uint8_t buf[2048];

    for (;;) {
        ++ctx.now_ms;
        endpoint.Tick();

        for (;;) {
            sockaddr_in from_addr;
            socklen_compat from_len = static_cast<socklen_compat>(sizeof(from_addr));
            const int n = recvfrom(sock,
#ifdef _WIN32
                                   reinterpret_cast<char*>(buf),
#else
                                   buf,
#endif
                                   sizeof(buf),
                                   0,
                                   reinterpret_cast<sockaddr*>(&from_addr),
                                   &from_len);
            if (n <= 0) {
                break;
            }
            endpoint.OnUdpPacket(buf, static_cast<uint16_t>(n));
        }

        const rudp::ConnectionState state = endpoint.GetConnectionState();
        if (state != last_state) {
            printf("[peer] state=%d\n", static_cast<int>(state));
            last_state = state;
        }

        if (endpoint.IsConnected() && ctx.now_ms - last_tx_ms >= 5000) {
            char msg[64];
            snprintf(msg, sizeof(msg), "host-ping-%u", static_cast<unsigned>(ctx.tx_count + 1));
            if (endpoint.Send(reinterpret_cast<const uint8_t*>(msg), static_cast<uint16_t>(strlen(msg) + 1)) == rudp::SendStatus::kOk) {
                ++ctx.tx_count;
                printf("[peer] sent %s\n", msg);
                last_tx_ms = ctx.now_ms;
            }
        }

#ifdef _WIN32
        Sleep(1);
#else
        usleep(1000);
#endif
    }

    return 0;
}