/**
 * @file esp32_udp.cpp
 * @brief ESP32 reference template for udplink with the current callback API.
 *
 * This file is intentionally lightweight: fill in your WiFi/Ethernet setup,
 * adjust the peer address below, and keep the udplink usage pattern unchanged.
 */

#include "rudp/rudp.hpp"

#include <stdint.h>
#include <string.h>

#ifdef RUDP_TEMPLATE_COMPILE_ONLY
#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

typedef void* TaskHandle_t;

static int64_t esp_timer_get_time() {
    return 0;
}

static unsigned pdMS_TO_TICKS(unsigned ms) {
    return ms;
}

static void vTaskDelay(unsigned) {}

static int xTaskCreate(void (*)(void*), const char*, unsigned, void*, unsigned, TaskHandle_t*) {
    return 0;
}

#define ESP_LOGI(tag, fmt, ...) do { (void)(tag); } while (0)
#define ESP_LOGE(tag, fmt, ...) do { (void)(tag); } while (0)
#else
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/sockets.h>
#endif

namespace {

#if defined(RUDP_TEMPLATE_COMPILE_ONLY) && defined(_WIN32)
typedef SOCKET UdpSocketHandle;
static const UdpSocketHandle kInvalidUdpSocket = INVALID_SOCKET;
#else
typedef int UdpSocketHandle;
static const UdpSocketHandle kInvalidUdpSocket = -1;
#endif

static const char* kTag = "udplink_esp32";
static const char* kPeerIp = "192.168.1.100";
static const uint16_t kLocalPort = 8888;
static const uint16_t kPeerPort = 8889;

static rudp::Endpoint g_endpoint;
static UdpSocketHandle g_udp_sock = kInvalidUdpSocket;
static sockaddr_in g_peer_addr;

static uint32_t NowMs(void*) {
    return static_cast<uint32_t>(esp_timer_get_time() / 1000ull);
}

static bool SendRaw(void*, const uint8_t* data, uint16_t len) {
    const int sent = sendto(g_udp_sock,
                            reinterpret_cast<const char*>(data),
                            static_cast<int>(len),
                            0,
                            reinterpret_cast<const sockaddr*>(&g_peer_addr),
                            sizeof(g_peer_addr));
    return sent == static_cast<int>(len);
}

static bool SendRawVec(void*,
                       const uint8_t* head, uint16_t head_len,
                       const uint8_t* body, uint16_t body_len) {
    uint8_t frame[rudp::Endpoint::kMaxFrame];
    if (static_cast<uint32_t>(head_len) + static_cast<uint32_t>(body_len) > sizeof(frame)) {
        return false;
    }
    memcpy(frame, head, head_len);
    memcpy(frame + head_len, body, body_len);
    return SendRaw(0, frame, static_cast<uint16_t>(head_len + body_len));
}

static void OnDeliver(void*, const uint8_t* data, uint16_t len) {
    ESP_LOGI(kTag, "received %u bytes: %.*s",
             static_cast<unsigned>(len),
             static_cast<int>(len),
             reinterpret_cast<const char*>(data));
}

static void ReceiveTask(void*) {
    uint8_t buffer[2048];
    while (true) {
        const int n = recvfrom(g_udp_sock,
                               reinterpret_cast<char*>(buffer),
                               sizeof(buffer),
                               0,
                               0,
                               0);
        if (n > 0) {
            g_endpoint.OnUdpPacket(buffer, static_cast<uint16_t>(n));
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

}  // namespace

extern "C" void app_main() {
    g_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_udp_sock == kInvalidUdpSocket) {
        ESP_LOGE(kTag, "failed to create UDP socket");
        return;
    }

    sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(kLocalPort);
    if (bind(g_udp_sock, reinterpret_cast<sockaddr*>(&local_addr), sizeof(local_addr)) < 0) {
        ESP_LOGE(kTag, "failed to bind local port %u", static_cast<unsigned>(kLocalPort));
        return;
    }

    memset(&g_peer_addr, 0, sizeof(g_peer_addr));
    g_peer_addr.sin_family = AF_INET;
    g_peer_addr.sin_port = htons(kPeerPort);
    inet_pton(AF_INET, kPeerIp, &g_peer_addr.sin_addr);

    rudp::Config cfg = rudp::ConfigForProfile(rudp::ConfigProfile::kLowPower);
    cfg.enable_auth = true;
    cfg.auth_key0 = 0x0123456789ABCDEFull;
    cfg.auth_key1 = 0xFEDCBA9876543210ull;

    rudp::Hooks hooks = {0, NowMs, SendRaw, SendRawVec, OnDeliver, 0, 0};
    if (!g_endpoint.Init(cfg, hooks)) {
        ESP_LOGE(kTag, "rudp init failed");
        return;
    }
    if (!g_endpoint.StartConnect()) {
        ESP_LOGE(kTag, "rudp connect start failed");
        return;
    }

    xTaskCreate(ReceiveTask, "udplink_rx", 4096, 0, 5, 0);

    rudp::ConnectionState last_state = rudp::ConnectionState::kDisconnected;
    while (true) {
        g_endpoint.Tick();

        const rudp::ConnectionState state = g_endpoint.GetConnectionState();
        if (state != last_state) {
            ESP_LOGI(kTag, "state=%u", static_cast<unsigned>(state));
            last_state = state;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
