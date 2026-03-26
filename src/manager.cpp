#include "rudp/manager.hpp"

#include <string.h>

namespace rudp {

ConnectionManager::ConnectionManager() : initialized_(false) {
    memset(&endpoint_cfg_, 0, sizeof(endpoint_cfg_));
    memset(&hooks_, 0, sizeof(hooks_));
    memset(slots_, 0, sizeof(slots_));
    for (uint16_t i = 0; i < kMaxConnections; ++i) {
        slots_[i].ctx.mgr = this;
        slots_[i].ctx.idx = i;
    }
}

bool ConnectionManager::Init(const Config& endpoint_cfg, const ManagerHooks& hooks) {
    if (hooks.now_ms == 0 || hooks.send_raw == 0 || hooks.on_deliver == 0) {
        return false;
    }
    endpoint_cfg_ = endpoint_cfg;
    hooks_ = hooks;
    initialized_ = true;
    return true;
}

void ConnectionManager::Lock() const {
    if (hooks_.enter_critical) {
        hooks_.enter_critical(hooks_.user);
    }
}

void ConnectionManager::Unlock() const {
    if (hooks_.leave_critical) {
        hooks_.leave_critical(hooks_.user);
    }
}

ConnectionManager::Slot* ConnectionManager::FindSlot(uint64_t key) {
    for (uint16_t i = 0; i < kMaxConnections; ++i) {
        if (slots_[i].used && slots_[i].key == key) {
            return &slots_[i];
        }
    }
    return 0;
}

const ConnectionManager::Slot* ConnectionManager::FindSlot(uint64_t key) const {
    for (uint16_t i = 0; i < kMaxConnections; ++i) {
        if (slots_[i].used && slots_[i].key == key) {
            return &slots_[i];
        }
    }
    return 0;
}

ConnectionManager::Slot* ConnectionManager::AllocateSlot(uint64_t key) {
    for (uint16_t i = 0; i < kMaxConnections; ++i) {
        if (!slots_[i].used) {
            slots_[i].used = true;
            slots_[i].key = key;
            return &slots_[i];
        }
    }
    return 0;
}

ConnectionManager::Slot* ConnectionManager::EnsureSlot(uint64_t key) {
    Slot* slot = FindSlot(key);
    if (slot) {
        return slot;
    }
    slot = AllocateSlot(key);
    if (!slot) {
        return 0;
    }

    Hooks h;
    h.user = &slot->ctx;
    h.now_ms = NowMsThunk;
    h.send_raw = SendRawThunk;
    h.send_raw_vec = SendRawVecThunk;
    h.on_deliver = DeliverThunk;
    // Manager-level lock already protects endpoint access.
    h.enter_critical = 0;
    h.leave_critical = 0;
    if (!slot->ep.Init(endpoint_cfg_, h)) {
        slot->used = false;
        slot->key = 0;
        return 0;
    }
    return slot;
}

void ConnectionManager::FreeSlot(Slot* slot) {
    if (!slot) {
        return;
    }
    slot->ep.Disconnect();
    slot->used = false;
    slot->key = 0;
}

uint32_t ConnectionManager::NowMsThunk(void* user) {
    SlotCtx* ctx = static_cast<SlotCtx*>(user);
    return ctx->mgr->hooks_.now_ms(ctx->mgr->hooks_.user);
}

bool ConnectionManager::SendRawThunk(void* user, const uint8_t* data, uint16_t len) {
    SlotCtx* ctx = static_cast<SlotCtx*>(user);
    const Slot* slot = &ctx->mgr->slots_[ctx->idx];
    if (!slot->used) {
        return false;
    }
    return ctx->mgr->hooks_.send_raw(ctx->mgr->hooks_.user, slot->key, data, len);
}

bool ConnectionManager::SendRawVecThunk(void* user,
                                        const uint8_t* head, uint16_t head_len,
                                        const uint8_t* body, uint16_t body_len) {
    SlotCtx* ctx = static_cast<SlotCtx*>(user);
    const Slot* slot = &ctx->mgr->slots_[ctx->idx];
    if (!slot->used) {
        return false;
    }
    if (ctx->mgr->hooks_.send_raw_vec) {
        return ctx->mgr->hooks_.send_raw_vec(ctx->mgr->hooks_.user, slot->key, head, head_len, body, body_len);
    }
    uint8_t tmp[Endpoint::kMaxFrame];
    const uint16_t n = static_cast<uint16_t>(head_len + body_len);
    if (n > Endpoint::kMaxFrame) {
        return false;
    }
    memcpy(tmp, head, head_len);
    memcpy(tmp + head_len, body, body_len);
    return ctx->mgr->hooks_.send_raw(ctx->mgr->hooks_.user, slot->key, tmp, n);
}

void ConnectionManager::DeliverThunk(void* user, const uint8_t* data, uint16_t len) {
    SlotCtx* ctx = static_cast<SlotCtx*>(user);
    const Slot* slot = &ctx->mgr->slots_[ctx->idx];
    if (!slot->used) {
        return;
    }
    ctx->mgr->hooks_.on_deliver(ctx->mgr->hooks_.user, slot->key, data, len);
}

Endpoint* ConnectionManager::Open(uint64_t key, bool start_connect) {
    Lock();
    if (!initialized_) {
        Unlock();
        return 0;
    }
    Slot* slot = EnsureSlot(key);
    if (!slot) {
        Unlock();
        return 0;
    }
    if (start_connect) {
        (void)slot->ep.StartConnect();
    }
    Endpoint* ep = &slot->ep;
    Unlock();
    return ep;
}

Endpoint* ConnectionManager::Find(uint64_t key) {
    Lock();
    Slot* slot = FindSlot(key);
    Endpoint* ep = slot ? &slot->ep : 0;
    Unlock();
    return ep;
}

void ConnectionManager::Remove(uint64_t key) {
    Lock();
    Slot* slot = FindSlot(key);
    if (slot) {
        FreeSlot(slot);
    }
    Unlock();
}

void ConnectionManager::OnUdpPacket(uint64_t key, const uint8_t* data, uint16_t len) {
    Lock();
    if (!initialized_) {
        Unlock();
        return;
    }
    Slot* slot = EnsureSlot(key);
    if (!slot) {
        Unlock();
        return;
    }
    slot->ep.OnUdpPacket(data, len);
    Unlock();
}

void ConnectionManager::Tick() {
    Lock();
    if (!initialized_) {
        Unlock();
        return;
    }
    for (uint16_t i = 0; i < kMaxConnections; ++i) {
        if (slots_[i].used) {
            slots_[i].ep.Tick();
        }
    }
    Unlock();
}

SendStatus ConnectionManager::Send(uint64_t key, const uint8_t* payload, uint16_t len) {
    Lock();
    if (!initialized_) {
        Unlock();
        return SendStatus::kQueueFull;
    }
    Slot* slot = EnsureSlot(key);
    if (!slot) {
        Unlock();
        return SendStatus::kQueueFull;
    }
    const SendStatus st = slot->ep.Send(payload, len);
    Unlock();
    return st;
}

SendStatus ConnectionManager::SendZeroCopy(uint64_t key, const uint8_t* payload, uint16_t len) {
    Lock();
    if (!initialized_) {
        Unlock();
        return SendStatus::kQueueFull;
    }
    Slot* slot = EnsureSlot(key);
    if (!slot) {
        Unlock();
        return SendStatus::kQueueFull;
    }
    const SendStatus st = slot->ep.SendZeroCopy(payload, len);
    Unlock();
    return st;
}

bool ConnectionManager::IsConnected(uint64_t key) const {
    Lock();
    const Slot* slot = FindSlot(key);
    const bool connected = (slot != 0) && slot->ep.IsConnected();
    Unlock();
    return connected;
}

ConnectionState ConnectionManager::GetConnectionState(uint64_t key) const {
    Lock();
    const Slot* slot = FindSlot(key);
    const ConnectionState st = slot ? slot->ep.GetConnectionState() : ConnectionState::kDisconnected;
    Unlock();
    return st;
}

bool ConnectionManager::GetStats(uint64_t key, Stats* out) const {
    if (!out) {
        return false;
    }
    Lock();
    const Slot* slot = FindSlot(key);
    if (!slot) {
        Unlock();
        return false;
    }
    *out = slot->ep.GetStats();
    Unlock();
    return true;
}

bool ConnectionManager::GetRuntimeMetrics(uint64_t key, RuntimeMetrics* out) const {
    if (!out) {
        return false;
    }
    Lock();
    const Slot* slot = FindSlot(key);
    if (!slot) {
        Unlock();
        return false;
    }
    *out = slot->ep.GetRuntimeMetrics();
    Unlock();
    return true;
}

uint16_t ConnectionManager::GetActiveCount() const {
    Lock();
    uint16_t n = 0;
    for (uint16_t i = 0; i < kMaxConnections; ++i) {
        if (slots_[i].used) {
            ++n;
        }
    }
    Unlock();
    return n;
}

}  // namespace rudp
