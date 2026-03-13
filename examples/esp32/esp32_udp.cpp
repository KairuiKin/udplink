/**
 * @file esp32_udp.cpp
 * @brief ESP32 UDP Example for udplink
 * 
 * This example demonstrates how to use udplink on ESP32.
 * Hardware: ESP32 + Ethernet/WiFi
 * 
 * @note Requires ESP-IDF and lwIP stack
 */

#include "rudp/rudp.hpp"
#include <cstring>
#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_eth.h>
#include <lwip/sockets.h>
#include <lwip/netif.h>

static const char* TAG = "udplink_example";

namespace {

constexpr uint16_t kLocalPort = 8888;
constexpr uint16_t kRemotePort = 8889;

// Auth keys
constexpr uint64_t kAuthKey0 = 0x0123456789ABCDEF;
constexpr uint64_t kAuthKey1 = 0xFEDCBA9876543210;

rudp::Endpoint* g_endpoint = nullptr;
int g_udp_sock = -1;
TaskHandle_t g_task_handle = nullptr;

// Send UDP packet
int UDP_Send(const uint8_t* data, uint32_t len, void* ctx) {
    if (g_udp_sock < 0) return -1;
    
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(kRemotePort);
    inet_pton(AF_INET, "192.168.1.100", &dest_addr.sin_addr.s_addr);
    
    int sent = sendto(g_udp_sock, data, len, 0,
                      (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    return (sent > 0) ? sent : -1;
}

// Get tick in milliseconds
uint64_t GetTickMs() {
    return esp_timer_get_time() / 1000;
}

// UDP receive task
void UDP_Task(void* param) {
    uint8_t buffer[2048];
    struct sockaddr_in src_addr;
    socklen_t addr_len = sizeof(src_addr);
    
    while (true) {
        int len = recvfrom(g_udp_sock, buffer, sizeof(buffer), 0,
                          (struct sockaddr*)&src_addr, &addr_len);
        if (len > 0 && g_endpoint) {
            g_endpoint->OnUdpPacket(buffer, len);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

}  // namespace

extern "C" void app_main() {
    ESP_LOGI(TAG, "Starting udplink on ESP32...");
    
    // Create UDP socket
    g_udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_udp_sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket");
        return;
    }
    
    // Bind to local port
    struct sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(kLocalPort);
    
    if (bind(g_udp_sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        ESP_LOGE(TAG, "Failed to bind socket");
        close(g_udp_sock);
        return;
    }
    
    // Set receive timeout
    struct timeval tv = { .tv_sec = 0, .tv_usec = 100000 };
    setsockopt(g_udp_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    // Configure udplink
    rudp::Config config = rudp::ConfigForProfile(rudp::kProfileLowPower);
    config.enable_auth = true;
    config.auth_key0 = kAuthKey0;
    config.auth_key1 = kAuthKey1;
    
    // Callbacks
    rudp::Hooks hooks;
    hooks.send = [](const uint8_t* data, uint32_t len, void*) -> int {
        return UDP_Send(data, len, nullptr);
    };
    hooks.get_tick_ms = []() -> uint64_t {
        return GetTickMs();
    };
    hooks.on_connected = []() {
        ESP_LOGI(TAG, "Connected!");
    };
    hooks.on_disconnected = []() {
        ESP_LOGI(TAG, "Disconnected");
    };
    hooks.on_message = [](const uint8_t* data, uint32_t len) {
        ESP_LOGI(TAG, "Received: %.*s", len, data);
    };
    
    // Create endpoint
    g_endpoint = new rudp::Endpoint();
    g_endpoint->Init(config, hooks);
    
    // Start connection
    g_endpoint->StartConnect("192.168.1.100", kRemotePort);
    
    // Start receive task
    xTaskCreate(UDP_Task, "udp_rx", 4096, nullptr, 5, &g_task_handle);
    
    // Main loop - tick every 10ms
    while (true) {
        if (g_endpoint) {
            g_endpoint->Tick();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    // Cleanup
    if (g_endpoint) delete g_endpoint;
    if (g_udp_sock >= 0) close(g_udp_sock);
}
