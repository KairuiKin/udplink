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

struct SensorData {
    float temperature;
    float humidity;
    uint32_t timestamp_ms;
};

static uint32_t NowMs(void* u) {
    return static_cast<PeerCtx*>(u)->now_ms;
}

static bool SendRaw(void* u, const uint8_t* data, uint16_t len) {
    PeerCtx* ctx = static_cast<PeerCtx*>(u);
    const ssize_t n = sendto(ctx->sock,
                             data,
                             len,
                             0,
                             reinterpret_cast<const sockaddr*>(&ctx->peer_addr),
                             sizeof(ctx->peer_addr));
    return n == static_cast<ssize_t>(len);
}

static void OnDeliver(void* u, const uint8_t* data, uint16_t len) {
    PeerCtx* ctx = static_cast<PeerCtx*>(u);
    if (len == sizeof(SensorData)) {
        const SensorData* sensor = reinterpret_cast<const SensorData*>(data);
        printf("[%s] sensor: temp=%.1fC humidity=%.1f%% ts=%u\n",
               ctx->tag,
               sensor->temperature,
               sensor->humidity,
               static_cast<unsigned>(sensor->timestamp_ms));
        return;
    }
    printf("[%s] recv %u bytes\n", ctx->tag, static_cast<unsigned>(len));
}

static bool MakeNonBlocking(int s) {
    const int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    return fcntl(s, F_SETFL, flags | O_NONBLOCK) == 0;
}

int main(int argc, char** argv) {
    const bool is_client = argc > 1 && strcmp(argv[1], "client") == 0;
    const bool is_server = argc > 1 && strcmp(argv[1], "server") == 0;
    if (!is_client && !is_server) {
        fprintf(stderr, "usage: %s <client|server> [peer_ip]\n", argv[0]);
        return 1;
    }

    const char* peer_ip = argc > 2 ? argv[2] : "127.0.0.1";
    const uint16_t local_port = is_client ? 29001 : 29002;
    const uint16_t peer_port = is_client ? 29002 : 29001;

    const int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        return 2;
    }

    sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(local_port);
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, reinterpret_cast<sockaddr*>(&local_addr), sizeof(local_addr)) != 0) {
        close(sock);
        return 3;
    }
    if (!MakeNonBlocking(sock)) {
        close(sock);
        return 4;
    }

    sockaddr_in peer_addr;
    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(peer_port);
    inet_pton(AF_INET, peer_ip, &peer_addr.sin_addr);

    PeerCtx ctx = {sock, peer_addr, 1, is_client ? "client" : "server"};

    rudp::Config cfg = rudp::ConfigForProfile(rudp::ConfigProfile::kBalanced);
    cfg.enable_auth = true;
    cfg.auth_key0 = 0x0706050403020100ull;
    cfg.auth_key1 = 0x0F0E0D0C0B0A0908ull;

    rudp::Endpoint endpoint;
    rudp::Hooks hooks = {&ctx, NowMs, SendRaw, 0, OnDeliver, 0, 0};
    if (!endpoint.Init(cfg, hooks)) {
        close(sock);
        return 5;
    }
    if (is_client && !endpoint.StartConnect()) {
        close(sock);
        return 6;
    }

    bool sent_sensor = false;
    rudp::ConnectionState last_state = rudp::ConnectionState::kDisconnected;
    uint8_t buf[2048];
    for (int i = 0; i < 12000; ++i) {
        ++ctx.now_ms;
        endpoint.Tick();

        for (;;) {
            const ssize_t n = recvfrom(sock, buf, sizeof(buf), 0, 0, 0);
            if (n <= 0) {
                break;
            }
            endpoint.OnUdpPacket(buf, static_cast<uint16_t>(n));
        }

        const rudp::ConnectionState state = endpoint.GetConnectionState();
        if (state != last_state) {
            printf("[%s] state=%u\n", ctx.tag, static_cast<unsigned>(state));
            last_state = state;
        }

        if (is_client && endpoint.IsConnected() && !sent_sensor) {
            SensorData sensor = {23.5f, 48.0f, ctx.now_ms};
            if (endpoint.Send(reinterpret_cast<const uint8_t*>(&sensor), sizeof(sensor)) == rudp::SendStatus::kOk) {
                sent_sensor = true;
            }
        }
        usleep(1000);
    }

    close(sock);
    return 0;
}

#else
int main() { return 0; }
#endif
