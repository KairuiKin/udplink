#ifndef RUDP_RUDP_HPP
#define RUDP_RUDP_HPP

#include <stddef.h>
#include <stdint.h>

namespace rudp {

enum class SendStatus : uint8_t {
    kOk = 0,
    kQueueFull = 1,
    kPayloadTooLarge = 2,
    kSequenceSpaceExhausted = 3,
    kNotConnected = 4,
    kUnsupported = 5
};

enum class ConnectionState : uint8_t {
    kDisconnected = 0,
    kConnecting = 1,
    kConnected = 2,
    kTimedOut = 3
};

enum class ConfigProfile : uint8_t {
    kBalanced = 0,
    kLowLatency = 1,
    kLowPower = 2,
    kLossyLink = 3
};

struct Stats {
    uint32_t tx_packets;
    uint32_t tx_retransmits;
    uint32_t tx_acks;
    uint32_t tx_control;
    uint32_t tx_dropped_by_pacing;
    uint32_t rx_packets;
    uint32_t rx_dropped;
    uint32_t rx_delivered;
    uint32_t rx_duplicates;
    uint32_t rx_control;
    uint32_t connect_attempts;
    uint32_t connect_success;
    uint32_t connect_timeouts;
};

struct RuntimeMetrics {
    uint16_t pending_send;
    uint32_t rto_ms;
    uint32_t smoothed_rtt_ms;
    uint32_t rtt_var_ms;
    uint32_t tx_retransmit_ratio_permille;
    uint32_t rx_drop_ratio_permille;
};

struct Config {
    uint16_t mtu;
    uint16_t max_payload;
    uint16_t send_window;
    uint16_t recv_window;
    uint16_t retransmit_min_ms;
    uint16_t retransmit_max_ms;
    uint16_t ack_delay_ms;
    uint16_t connect_retry_ms;
    uint16_t idle_timeout_ms;
    uint16_t heartbeat_ms;
    uint16_t pacing_bytes_per_tick;
    uint16_t key_update_retry_ms;
    uint8_t max_retransmits;
    uint8_t max_connect_retries;
    uint8_t key_update_max_retries;
    bool enable_auth;
    uint32_t auth_psk;
    uint64_t auth_key0;
    uint64_t auth_key1;
};

typedef uint32_t (*NowMsFn)(void* user);
typedef bool (*SendRawFn)(void* user, const uint8_t* data, uint16_t len);
typedef bool (*SendRawVecFn)(void* user,
                             const uint8_t* head, uint16_t head_len,
                             const uint8_t* body, uint16_t body_len);
typedef void (*OnDeliverFn)(void* user, const uint8_t* data, uint16_t len);
typedef void (*CriticalFn)(void* user);

struct Hooks {
    void* user;
    NowMsFn now_ms;
    SendRawFn send_raw;
    SendRawVecFn send_raw_vec;
    OnDeliverFn on_deliver;
    CriticalFn enter_critical;
    CriticalFn leave_critical;
};

class Endpoint {
public:
    static const uint16_t kMaxFrame = 512;
    static const uint16_t kMaxQueue = 64;
    static const uint16_t kAckBitmapBytes = 4;

    Endpoint();

    bool Init(const Config& cfg, const Hooks& hooks);
    bool SetAuthKey(uint8_t key_id, uint64_t key0, uint64_t key1, bool set_as_tx);
    bool RotateTxKey(uint8_t key_id);
    bool ScheduleTxKeyRotation(uint8_t key_id, uint16_t lead_packets);
    bool StartConnect();
    void ForceConnectedForTest();
    void Disconnect();
    SendStatus Send(const uint8_t* payload, uint16_t len);
    SendStatus SendZeroCopy(const uint8_t* payload, uint16_t len);
    void OnUdpPacket(const uint8_t* data, uint16_t len);
    void Tick();

    const Stats& GetStats() const { return stats_; }
    RuntimeMetrics GetRuntimeMetrics() const;
    ConnectionState GetConnectionState() const { return conn_state_; }
    bool IsConnected() const { return conn_state_ == ConnectionState::kConnected; }
    uint16_t GetPendingSend() const;

private:
    void Lock();
    void Unlock();

    enum : uint8_t {
        kFlagAckOnly = 0x01,
        kFlagSyn = 0x02,
        kFlagSynAck = 0x04,
        kFlagPing = 0x08,
        kFlagKeyUpdate = 0x10,
        kFlagKeyUpdateAck = 0x20
    };

    struct PacketSlot {
        bool used;
        bool acked;
        bool ever_sent;
        bool zero_copy;
        uint8_t retries;
        uint16_t seq;
        uint16_t payload_len;
        uint16_t frame_len;
        uint32_t last_sent_ms;
        const uint8_t* ext_payload;
        uint32_t nonce;
        uint64_t auth_tag;
        uint8_t frame[kMaxFrame];
    };

    struct RecvSlot {
        bool used;
        uint16_t seq;
        uint16_t len;
        uint8_t data[kMaxFrame];
    };

    struct WireHeader {
        uint16_t magic;
        uint16_t seq;
        uint16_t ack;
        uint8_t ack_bits[kAckBitmapBytes];
        uint32_t session_id;
        uint32_t nonce;
        uint16_t payload_len;
        uint8_t flags;
        uint8_t key_id;
        uint64_t auth_tag;
    };

    bool BuildDataFrame(PacketSlot* slot, uint16_t seq, const uint8_t* payload, uint16_t len, bool zero_copy);
    bool BuildControlFrame(uint8_t* out, uint16_t* out_len,
                           uint8_t flags, uint32_t nonce,
                           const uint8_t* payload, uint16_t payload_len) const;
    uint64_t ComputeAuthTag(const uint8_t* head_no_tag, uint16_t head_no_tag_len,
                            const uint8_t* payload, uint16_t payload_len,
                            uint32_t nonce, uint8_t key_id) const;
    bool ResolveAuthKey(uint8_t key_id, uint64_t* k0, uint64_t* k1) const;
    void InitAuthKeyRing();
    bool ValidateAndTrackReplay(uint32_t nonce, bool* duplicate);
    bool SendControl(uint8_t flags, uint32_t now_ms,
                     const uint8_t* payload, uint16_t payload_len);
    bool SendFrame(PacketSlot* slot, uint32_t now_ms, bool is_retransmit);
    bool ConsumePacingBudget(uint16_t bytes);
    void FlushAcks(uint32_t now_ms, bool force);
    void HandleAck(uint16_t ack, uint32_t ack_bits);
    void FastRetransmit(uint32_t now_ms);
    void HandleControl(uint8_t flags, uint32_t sender_session_id, uint32_t now_ms,
                       const uint8_t* payload, uint16_t payload_len);
    void InsertRecv(uint16_t seq, const uint8_t* payload, uint16_t len);
    void DrainInOrder();
    bool IsSeqLess(uint16_t a, uint16_t b) const;
    bool SeqInWindow(uint16_t seq, uint16_t start, uint16_t window) const;
    uint32_t BuildAckBits(uint16_t newest) const;
    uint16_t DistanceFrom(uint16_t older, uint16_t newer) const;
    PacketSlot* FindSendSlotBySeq(uint16_t seq);
    RecvSlot* FindRecvSlotBySeq(uint16_t seq);
    PacketSlot* AllocateSendSlot();
    RecvSlot* AllocateRecvSlot();
    void MaybeAdjustRto(uint32_t sample_ms);

    Config cfg_;
    Hooks hooks_;
    bool initialized_;

    uint16_t next_send_seq_;
    uint16_t recv_next_seq_;
    uint16_t newest_recv_seq_;
    bool has_recv_seq_;

    uint32_t smoothed_rtt_;
    uint32_t rtt_var_;
    uint32_t rto_ms_;

    uint32_t last_ack_flush_ms_;
    uint32_t last_rx_ms_;
    uint32_t last_tx_ms_;
    uint32_t last_connect_try_ms_;
    uint16_t last_seen_ack_;
    uint32_t last_seen_ack_bits_;
    uint8_t dup_ack_count_;
    uint16_t pacing_budget_bytes_;
    uint8_t connect_retries_;
    struct AuthKeySlot {
        bool used;
        uint8_t key_id;
        uint64_t key0;
        uint64_t key1;
    };
    AuthKeySlot auth_keys_[2];
    uint8_t tx_key_id_;
    bool pending_tx_key_rotation_;
    uint8_t pending_tx_key_id_;
    uint32_t pending_tx_activate_nonce_;
    bool pending_tx_key_acknowledged_;
    uint8_t pending_tx_old_key_id_;
    uint8_t pending_tx_key_retry_count_;
    uint32_t last_key_update_announce_ms_;
    bool peer_key_rotation_known_;
    uint8_t peer_next_key_id_;
    uint32_t peer_key_activate_nonce_;
    uint32_t local_session_id_;
    uint32_t peer_session_id_;
    uint32_t tx_nonce_counter_;
    uint32_t rx_replay_max_nonce_;
    uint64_t rx_replay_bitmap_;
    bool has_peer_session_id_;
    ConnectionState conn_state_;
    bool ack_dirty_;

    PacketSlot send_slots_[kMaxQueue];
    RecvSlot recv_slots_[kMaxQueue];
    Stats stats_;
};

Config DefaultConfig();
Config ConfigForProfile(ConfigProfile profile);

}  // namespace rudp

#endif
