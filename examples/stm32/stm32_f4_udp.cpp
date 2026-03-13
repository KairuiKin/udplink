/**
 * @file stm32_f4_udp.cpp
 * @brief STM32F4 UDP Example for udplink
 * 
 * This example demonstrates how to use udplink on STM32F4 MCU.
 * Hardware: STM32F4xx + Ethernet (W5500/LWIP)
 * 
 * @note Requires STM32 HAL and LWIP stack
 */

#include "rudp/rudp.hpp"
#include <cstring>
#include <cstdint>

// STM32 HAL includes (placeholder)
// #include "stm32f4xx_hal.h"
// #include "lwip/lwip.h"

namespace {

// Configuration for STM32F4
constexpr uint16_t kLocalPort = 8888;
constexpr uint16_t kRemotePort = 8889;
constexpr uint32_t kTickIntervalMs = 10;  // 10ms tick

// Auth keys (in production, store in secure storage)
constexpr uint64_t kAuthKey0 = 0x0123456789ABCDEF;
constexpr uint64_t kAuthKey1 = 0xFEDCBA9876543210;

// Global endpoint
rudp::Endpoint* g_endpoint = nullptr;

// UDP send callback - implement with your network stack
int UDP_Send(const uint8_t* data, uint32_t len, void* ctx) {
    // Example: using LWIP
    // struct pbuf* p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    // if (p) {
    //     memcpy(p->payload, data, len);
    //     udp_send(ctx, p);
    //     pbuf_free(p);
    //     return len;
    // }
    (void)data;
    (void)len;
    (void)ctx;
    return 0;
}

// Tick callback - return current time in milliseconds
uint64_t GetTickMs() {
    // Example: using HAL
    // return HAL_GetTick();
    return 0;
}

}  // namespace

/**
 * @brief Initialize hardware (Ethernet, UDP)
 */
void Hardware_Init() {
    // Initialize HAL
    // MX_LWIP_Init();
    
    // Initialize UDP socket
    // udp_new();
    // udp_bind(pcb, IP_ADDR_ANY, kLocalPort);
}

/**
 * @brief Process incoming UDP packets
 */
void OnUdpReceive(const uint8_t* data, uint32_t len) {
    if (g_endpoint) {
        g_endpoint->OnUdpPacket(data, len);
    }
}

/**
 * @brief Main application
 */
int main() {
    Hardware_Init();
    
    // Configure udplink
    rudp::Config config = rudp::ConfigForProfile(rudp::kProfileLowPower);
    config.enable_auth = true;
    config.auth_key0 = kAuthKey0;
    config.auth_key1 = kAuthKey1;
    
    // Setup callbacks
    rudp::Hooks hooks;
    hooks.send = [](const uint8_t* data, uint32_t len, void*) -> int {
        return UDP_Send(data, len, nullptr);
    };
    hooks.get_tick_ms = []() -> uint64_t {
        return GetTickMs();
    };
    hooks.on_connected = []() {
        // LED on when connected
        // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    };
    hooks.on_disconnected = []() {
        // LED off when disconnected
        // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    };
    hooks.on_message = [](const uint8_t* data, uint32_t len) {
        // Process received message
    };
    
    // Create endpoint
    g_endpoint = new rudp::Endpoint();
    g_endpoint->Init(config, hooks);
    
    // Start connection to remote
    // g_endpoint->StartConnect("192.168.1.100", kRemotePort);
    
    // Or wait for incoming connections
    // g_endpoint->StartListen(kLocalPort);
    
    // Main loop
    while (true) {
        // Process network stack
        // sys_check_timeouts();
        
        // Tick udplink
        if (g_endpoint) {
            g_endpoint->Tick();
        }
        
        // Delay
        // HAL_Delay(kTickIntervalMs);
    }
    
    delete g_endpoint;
    return 0;
}
