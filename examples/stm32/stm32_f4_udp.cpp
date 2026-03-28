/**
 * @file stm32_f4_udp.cpp
 * @brief STM32F4 reference template for udplink with the current callback API.
 *
 * Replace the placeholder HAL/LwIP hooks in this file with your board support
 * package. The udplink-facing code is the part intended to stay stable.
 */

#include "rudp/rudp.hpp"

#include <stdint.h>
#include <string.h>

namespace {

static const uint16_t kTickIntervalMs = 10;

static rudp::Endpoint g_endpoint;

static void HardwareInit() {
    // Initialize clocks, Ethernet/WiFi transport, and your UDP stack here.
}

static uint32_t PlatformNowMs() {
    // Replace with HAL_GetTick() or an equivalent board timer.
    return 0;
}

static bool PlatformSendUdp(const uint8_t* data, uint16_t len) {
    (void)data;
    (void)len;
    // Replace with lwIP/W5500 send implementation.
    return true;
}

static bool PlatformPollUdp(uint8_t* data, uint16_t* len) {
    (void)data;
    (void)len;
    // Replace with your non-blocking UDP receive path.
    return false;
}

static uint32_t NowMs(void*) {
    return PlatformNowMs();
}

static bool SendRaw(void*, const uint8_t* data, uint16_t len) {
    return PlatformSendUdp(data, len);
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
    return PlatformSendUdp(frame, static_cast<uint16_t>(head_len + body_len));
}

static void OnDeliver(void*, const uint8_t* data, uint16_t len) {
    (void)data;
    (void)len;
    // Forward the payload to your application task.
}

}  // namespace

int main() {
    HardwareInit();

    rudp::Config cfg = rudp::ConfigForProfile(rudp::ConfigProfile::kLowPower);
    cfg.enable_auth = true;
    cfg.auth_key0 = 0x0123456789ABCDEFull;
    cfg.auth_key1 = 0xFEDCBA9876543210ull;

    rudp::Hooks hooks = {0, NowMs, SendRaw, SendRawVec, OnDeliver, 0, 0};
    if (!g_endpoint.Init(cfg, hooks)) {
        return 1;
    }
    if (!g_endpoint.StartConnect()) {
        return 2;
    }

    uint8_t rx_buf[1536];
    for (;;) {
        uint16_t len = 0;
        if (PlatformPollUdp(rx_buf, &len) && len > 0) {
            g_endpoint.OnUdpPacket(rx_buf, len);
        }
        g_endpoint.Tick();

        // Sleep or wait on your scheduler for kTickIntervalMs.
        (void)kTickIntervalMs;
    }
}
