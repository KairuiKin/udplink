#ifndef RUDP_MANAGER_HPP
#define RUDP_MANAGER_HPP

#include "rudp/rudp.hpp"

#include <stddef.h>
#include <stdint.h>

namespace rudp {

typedef uint32_t (*ManagerNowMsFn)(void* user);
typedef bool (*ManagerSendRawFn)(void* user, uint64_t key, const uint8_t* data, uint16_t len);
typedef bool (*ManagerSendRawVecFn)(void* user,
                                    uint64_t key,
                                    const uint8_t* head, uint16_t head_len,
                                    const uint8_t* body, uint16_t body_len);
typedef void (*ManagerDeliverFn)(void* user, uint64_t key, const uint8_t* data, uint16_t len);

struct ManagerHooks {
    void* user;
    ManagerNowMsFn now_ms;
    ManagerSendRawFn send_raw;
    ManagerSendRawVecFn send_raw_vec;
    ManagerDeliverFn on_deliver;
    CriticalFn enter_critical;
    CriticalFn leave_critical;
};

class ConnectionManager {
public:
    static const uint16_t kMaxConnections = 8;

    ConnectionManager();

    bool Init(const Config& endpoint_cfg, const ManagerHooks& hooks);
    Endpoint* Open(uint64_t key, bool start_connect);
    Endpoint* Find(uint64_t key);
    void Remove(uint64_t key);
    void OnUdpPacket(uint64_t key, const uint8_t* data, uint16_t len);
    void Tick();
    SendStatus Send(uint64_t key, const uint8_t* payload, uint16_t len);
    SendStatus SendZeroCopy(uint64_t key, const uint8_t* payload, uint16_t len);
    uint16_t GetActiveCount() const;

private:
    struct SlotCtx {
        ConnectionManager* mgr;
        uint16_t idx;
    };

    struct Slot {
        bool used;
        uint64_t key;
        Endpoint ep;
        SlotCtx ctx;
    };

    static uint32_t NowMsThunk(void* user);
    static bool SendRawThunk(void* user, const uint8_t* data, uint16_t len);
    static bool SendRawVecThunk(void* user,
                                const uint8_t* head, uint16_t head_len,
                                const uint8_t* body, uint16_t body_len);
    static void DeliverThunk(void* user, const uint8_t* data, uint16_t len);

    Slot* FindSlot(uint64_t key);
    const Slot* FindSlot(uint64_t key) const;
    Slot* AllocateSlot(uint64_t key);
    void FreeSlot(Slot* slot);
    void Lock();
    void Unlock();

    bool initialized_;
    Config endpoint_cfg_;
    ManagerHooks hooks_;
    Slot slots_[kMaxConnections];
};

}  // namespace rudp

#endif
