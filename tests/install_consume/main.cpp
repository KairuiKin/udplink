#include "rudp/rudp.hpp"

#include <stdint.h>

static uint32_t NowMs(void*) {
    return 0;
}

static bool SendRaw(void*, const uint8_t*, uint16_t) {
    return true;
}

static void OnDeliver(void*, const uint8_t*, uint16_t) {}

int main() {
    rudp::Config cfg = rudp::ConfigForProfile(rudp::ConfigProfile::kBalanced);
    rudp::Hooks hooks = {0, NowMs, SendRaw, 0, OnDeliver, 0, 0};
    rudp::Endpoint endpoint;
    if (!endpoint.Init(cfg, hooks)) {
        return 1;
    }
    const rudp::RuntimeMetrics metrics = endpoint.GetRuntimeMetrics();
    return metrics.pending_send == 0 ? 0 : 2;
}
