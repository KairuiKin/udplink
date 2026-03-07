#include "rudp/rudp.hpp"

#include <string.h>

namespace rudp {

namespace {

static const uint16_t kMagic = 0xA55A;
static const uint16_t kHeaderLen = 30;
static const uint16_t kHeaderNoTagLen = 22;

static uint16_t ReadU16(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

static void WriteU16(uint8_t* p, uint16_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFFu);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
}

static uint32_t ReadU32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) |
           (static_cast<uint32_t>(p[3]) << 24);
}

static void WriteU32(uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFFu);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
    p[2] = static_cast<uint8_t>((v >> 16) & 0xFFu);
    p[3] = static_cast<uint8_t>((v >> 24) & 0xFFu);
}

static uint64_t ReadU64(const uint8_t* p) {
    return static_cast<uint64_t>(p[0]) |
           (static_cast<uint64_t>(p[1]) << 8) |
           (static_cast<uint64_t>(p[2]) << 16) |
           (static_cast<uint64_t>(p[3]) << 24) |
           (static_cast<uint64_t>(p[4]) << 32) |
           (static_cast<uint64_t>(p[5]) << 40) |
           (static_cast<uint64_t>(p[6]) << 48) |
           (static_cast<uint64_t>(p[7]) << 56);
}

static void WriteU64(uint8_t* p, uint64_t v) {
    p[0] = static_cast<uint8_t>(v & 0xFFu);
    p[1] = static_cast<uint8_t>((v >> 8) & 0xFFu);
    p[2] = static_cast<uint8_t>((v >> 16) & 0xFFu);
    p[3] = static_cast<uint8_t>((v >> 24) & 0xFFu);
    p[4] = static_cast<uint8_t>((v >> 32) & 0xFFu);
    p[5] = static_cast<uint8_t>((v >> 40) & 0xFFu);
    p[6] = static_cast<uint8_t>((v >> 48) & 0xFFu);
    p[7] = static_cast<uint8_t>((v >> 56) & 0xFFu);
}

static uint64_t RotL64(uint64_t x, uint8_t b) {
    return (x << b) | (x >> (64u - b));
}

static void SipRound(uint64_t* v0, uint64_t* v1, uint64_t* v2, uint64_t* v3) {
    *v0 += *v1;
    *v1 = RotL64(*v1, 13);
    *v1 ^= *v0;
    *v0 = RotL64(*v0, 32);
    *v2 += *v3;
    *v3 = RotL64(*v3, 16);
    *v3 ^= *v2;
    *v0 += *v3;
    *v3 = RotL64(*v3, 21);
    *v3 ^= *v0;
    *v2 += *v1;
    *v1 = RotL64(*v1, 17);
    *v1 ^= *v2;
    *v2 = RotL64(*v2, 32);
}

static uint64_t SipHash24(const uint8_t* data, uint16_t len, uint64_t k0, uint64_t k1) {
    uint64_t v0 = 0x736f6d6570736575ull ^ k0;
    uint64_t v1 = 0x646f72616e646f6dull ^ k1;
    uint64_t v2 = 0x6c7967656e657261ull ^ k0;
    uint64_t v3 = 0x7465646279746573ull ^ k1;
    const uint8_t* p = data;
    uint16_t left = len;
    while (left >= 8) {
        const uint64_t m = ReadU64(p);
        v3 ^= m;
        SipRound(&v0, &v1, &v2, &v3);
        SipRound(&v0, &v1, &v2, &v3);
        v0 ^= m;
        p += 8;
        left = static_cast<uint16_t>(left - 8);
    }
    uint64_t b = static_cast<uint64_t>(len) << 56;
    for (uint16_t i = 0; i < left; ++i) {
        b |= static_cast<uint64_t>(p[i]) << (8u * i);
    }
    v3 ^= b;
    SipRound(&v0, &v1, &v2, &v3);
    SipRound(&v0, &v1, &v2, &v3);
    v0 ^= b;
    v2 ^= 0xff;
    SipRound(&v0, &v1, &v2, &v3);
    SipRound(&v0, &v1, &v2, &v3);
    SipRound(&v0, &v1, &v2, &v3);
    SipRound(&v0, &v1, &v2, &v3);
    return v0 ^ v1 ^ v2 ^ v3;
}

}  // namespace

Config DefaultConfig() {
    Config c;
    c.mtu = 256;
    c.max_payload = 220;
    c.send_window = 32;
    c.recv_window = 32;
    c.retransmit_min_ms = 15;
    c.retransmit_max_ms = 1000;
    c.ack_delay_ms = 3;
    c.connect_retry_ms = 120;
    c.idle_timeout_ms = 6000;
    c.heartbeat_ms = 400;
    c.pacing_bytes_per_tick = 0;
    c.key_update_retry_ms = 120;
    c.max_retransmits = 15;
    c.max_connect_retries = 20;
    c.key_update_max_retries = 16;
    c.enable_auth = false;
    c.auth_psk = 0;
    c.auth_key0 = 0;
    c.auth_key1 = 0;
    return c;
}

Endpoint::Endpoint()
    : initialized_(false),
      next_send_seq_(1),
      recv_next_seq_(1),
      newest_recv_seq_(0),
      has_recv_seq_(false),
      smoothed_rtt_(0),
      rtt_var_(0),
      rto_ms_(80),
      last_ack_flush_ms_(0),
      last_rx_ms_(0),
      last_tx_ms_(0),
      last_connect_try_ms_(0),
      last_seen_ack_(0),
      last_seen_ack_bits_(0),
      dup_ack_count_(0),
      pacing_budget_bytes_(0),
      connect_retries_(0),
      local_session_id_(0),
      peer_session_id_(0),
      tx_nonce_counter_(0),
      rx_replay_max_nonce_(0),
      rx_replay_bitmap_(0),
      tx_key_id_(1),
      pending_tx_key_rotation_(false),
      pending_tx_key_id_(0),
      pending_tx_activate_nonce_(0),
      pending_tx_key_acknowledged_(false),
      pending_tx_old_key_id_(0),
      pending_tx_key_retry_count_(0),
      last_key_update_announce_ms_(0),
      peer_key_rotation_known_(false),
      peer_next_key_id_(0),
      peer_key_activate_nonce_(0),
      has_peer_session_id_(false),
      conn_state_(ConnectionState::kDisconnected),
      ack_dirty_(false) {
    memset(&cfg_, 0, sizeof(cfg_));
    memset(&hooks_, 0, sizeof(hooks_));
    memset(send_slots_, 0, sizeof(send_slots_));
    memset(recv_slots_, 0, sizeof(recv_slots_));
    memset(&stats_, 0, sizeof(stats_));
}

void Endpoint::InitAuthKeyRing() {
    for (uint8_t i = 0; i < 2; ++i) {
        auth_keys_[i].used = false;
        auth_keys_[i].key_id = 0;
        auth_keys_[i].key0 = 0;
        auth_keys_[i].key1 = 0;
    }
    tx_key_id_ = 1;
    uint64_t k0 = cfg_.auth_key0;
    uint64_t k1 = cfg_.auth_key1;
    if ((k0 | k1) == 0ull) {
        const uint64_t p = static_cast<uint64_t>(cfg_.auth_psk);
        k0 = (p << 32) | (p ^ 0xA5A5A5A5u);
        k1 = (~p << 32) | (p ^ 0x5A5A5A5Au);
    }
    if ((k0 | k1) != 0ull) {
        auth_keys_[0].used = true;
        auth_keys_[0].key_id = 1;
        auth_keys_[0].key0 = k0;
        auth_keys_[0].key1 = k1;
    }
}

bool Endpoint::ResolveAuthKey(uint8_t key_id, uint64_t* k0, uint64_t* k1) const {
    for (uint8_t i = 0; i < 2; ++i) {
        if (auth_keys_[i].used && auth_keys_[i].key_id == key_id) {
            if (k0) {
                *k0 = auth_keys_[i].key0;
            }
            if (k1) {
                *k1 = auth_keys_[i].key1;
            }
            return true;
        }
    }
    return false;
}

bool Endpoint::SetAuthKey(uint8_t key_id, uint64_t key0, uint64_t key1, bool set_as_tx) {
    if (key_id == 0 || (key0 | key1) == 0ull) {
        return false;
    }
    uint8_t idx = 2;
    for (uint8_t i = 0; i < 2; ++i) {
        if (auth_keys_[i].used && auth_keys_[i].key_id == key_id) {
            idx = i;
            break;
        }
    }
    if (idx == 2) {
        for (uint8_t i = 0; i < 2; ++i) {
            if (!auth_keys_[i].used) {
                idx = i;
                break;
            }
        }
    }
    if (idx == 2) {
        // Replace the non-active slot first to keep online rotation safe.
        idx = (auth_keys_[0].used && auth_keys_[0].key_id == tx_key_id_) ? 1 : 0;
    }
    auth_keys_[idx].used = true;
    auth_keys_[idx].key_id = key_id;
    auth_keys_[idx].key0 = key0;
    auth_keys_[idx].key1 = key1;
    if (set_as_tx) {
        tx_key_id_ = key_id;
    }
    return true;
}

bool Endpoint::RotateTxKey(uint8_t key_id) {
    uint64_t k0 = 0;
    uint64_t k1 = 0;
    if (!ResolveAuthKey(key_id, &k0, &k1)) {
        return false;
    }
    (void)k0;
    (void)k1;
    tx_key_id_ = key_id;
    return true;
}

bool Endpoint::ScheduleTxKeyRotation(uint8_t key_id, uint16_t lead_packets) {
    uint64_t k0 = 0;
    uint64_t k1 = 0;
    if (!ResolveAuthKey(key_id, &k0, &k1)) {
        return false;
    }
    (void)k0;
    (void)k1;
    if (lead_packets < 4) {
        lead_packets = 4;
    }
    pending_tx_key_rotation_ = true;
    pending_tx_key_id_ = key_id;
    pending_tx_activate_nonce_ = tx_nonce_counter_ + lead_packets;
    pending_tx_key_acknowledged_ = false;
    pending_tx_old_key_id_ = tx_key_id_;
    pending_tx_key_retry_count_ = 0;
    last_key_update_announce_ms_ = 0;
    return true;
}

bool Endpoint::Init(const Config& cfg, const Hooks& hooks) {
    if (hooks.now_ms == 0 || hooks.send_raw == 0 || hooks.on_deliver == 0) {
        return false;
    }
    if (cfg.max_payload == 0 || cfg.max_payload > (kMaxFrame - kHeaderLen)) {
        return false;
    }
    if (cfg.send_window == 0 || cfg.send_window > kMaxQueue ||
        cfg.recv_window == 0 || cfg.recv_window > kMaxQueue) {
        return false;
    }
    if (cfg.mtu < (kHeaderLen + 1) || cfg.mtu > kMaxFrame) {
        return false;
    }

    cfg_ = cfg;
    hooks_ = hooks;
    rto_ms_ = cfg_.retransmit_min_ms;
    last_ack_flush_ms_ = hooks_.now_ms(hooks_.user);
    last_rx_ms_ = last_ack_flush_ms_;
    last_tx_ms_ = last_ack_flush_ms_;
    last_connect_try_ms_ = 0;
    pacing_budget_bytes_ = cfg_.pacing_bytes_per_tick;
    connect_retries_ = 0;
    last_seen_ack_ = 0;
    last_seen_ack_bits_ = 0;
    dup_ack_count_ = 0;
    local_session_id_ = (last_ack_flush_ms_ ^ 0xC0DE1234u) + 1u;
    peer_session_id_ = 0;
    tx_nonce_counter_ = local_session_id_ ^ 0x13579BDFu;
    rx_replay_max_nonce_ = 0;
    rx_replay_bitmap_ = 0;
    InitAuthKeyRing();
    pending_tx_key_rotation_ = false;
    pending_tx_key_id_ = 0;
    pending_tx_activate_nonce_ = 0;
    pending_tx_key_acknowledged_ = false;
    pending_tx_old_key_id_ = 0;
    pending_tx_key_retry_count_ = 0;
    last_key_update_announce_ms_ = 0;
    peer_key_rotation_known_ = false;
    peer_next_key_id_ = 0;
    peer_key_activate_nonce_ = 0;
    has_peer_session_id_ = false;
    conn_state_ = ConnectionState::kDisconnected;
    ack_dirty_ = false;
    initialized_ = true;
    return true;
}

bool Endpoint::StartConnect() {
    if (!initialized_) {
        return false;
    }
    if (conn_state_ == ConnectionState::kConnected || conn_state_ == ConnectionState::kConnecting) {
        return true;
    }
    conn_state_ = ConnectionState::kConnecting;
    connect_retries_ = 0;
    const uint32_t now = hooks_.now_ms(hooks_.user);
    local_session_id_ = (now ^ (local_session_id_ * 1664525u) ^ 0x9E3779B9u) + 1u;
    has_peer_session_id_ = false;
    peer_session_id_ = 0;
    tx_nonce_counter_ = local_session_id_ ^ 0x13579BDFu;
    rx_replay_max_nonce_ = 0;
    rx_replay_bitmap_ = 0;
    pending_tx_key_rotation_ = false;
    pending_tx_key_id_ = 0;
    pending_tx_activate_nonce_ = 0;
    pending_tx_key_acknowledged_ = false;
    pending_tx_old_key_id_ = 0;
    pending_tx_key_retry_count_ = 0;
    last_key_update_announce_ms_ = 0;
    peer_key_rotation_known_ = false;
    peer_next_key_id_ = 0;
    peer_key_activate_nonce_ = 0;
    last_connect_try_ms_ = now;
    ++stats_.connect_attempts;
    (void)SendControl(kFlagSyn, now, 0, 0);
    return true;
}

void Endpoint::ForceConnectedForTest() {
    conn_state_ = ConnectionState::kConnected;
}

void Endpoint::Disconnect() {
    conn_state_ = ConnectionState::kDisconnected;
    has_recv_seq_ = false;
    has_peer_session_id_ = false;
    peer_session_id_ = 0;
    rx_replay_max_nonce_ = 0;
    rx_replay_bitmap_ = 0;
    pending_tx_key_rotation_ = false;
    pending_tx_key_acknowledged_ = false;
    peer_key_rotation_known_ = false;
    ack_dirty_ = false;
    for (uint16_t i = 0; i < kMaxQueue; ++i) {
        send_slots_[i].used = false;
        recv_slots_[i].used = false;
    }
}

bool Endpoint::IsSeqLess(uint16_t a, uint16_t b) const {
    return static_cast<int16_t>(a - b) < 0;
}

uint16_t Endpoint::DistanceFrom(uint16_t older, uint16_t newer) const {
    return static_cast<uint16_t>(newer - older);
}

bool Endpoint::SeqInWindow(uint16_t seq, uint16_t start, uint16_t window) const {
    return DistanceFrom(start, seq) < window;
}

Endpoint::PacketSlot* Endpoint::AllocateSendSlot() {
    for (uint16_t i = 0; i < kMaxQueue; ++i) {
        if (!send_slots_[i].used) {
            send_slots_[i].used = true;
            send_slots_[i].acked = false;
            send_slots_[i].ever_sent = false;
            send_slots_[i].zero_copy = false;
            send_slots_[i].retries = 0;
            send_slots_[i].payload_len = 0;
            send_slots_[i].frame_len = 0;
            send_slots_[i].last_sent_ms = 0;
            send_slots_[i].ext_payload = 0;
            return &send_slots_[i];
        }
    }
    return 0;
}

Endpoint::RecvSlot* Endpoint::AllocateRecvSlot() {
    for (uint16_t i = 0; i < kMaxQueue; ++i) {
        if (!recv_slots_[i].used) {
            recv_slots_[i].used = true;
            recv_slots_[i].len = 0;
            return &recv_slots_[i];
        }
    }
    return 0;
}

Endpoint::PacketSlot* Endpoint::FindSendSlotBySeq(uint16_t seq) {
    for (uint16_t i = 0; i < kMaxQueue; ++i) {
        if (send_slots_[i].used && send_slots_[i].seq == seq) {
            return &send_slots_[i];
        }
    }
    return 0;
}

Endpoint::RecvSlot* Endpoint::FindRecvSlotBySeq(uint16_t seq) {
    for (uint16_t i = 0; i < kMaxQueue; ++i) {
        if (recv_slots_[i].used && recv_slots_[i].seq == seq) {
            return &recv_slots_[i];
        }
    }
    return 0;
}

bool Endpoint::BuildDataFrame(PacketSlot* slot, uint16_t seq, const uint8_t* payload, uint16_t len, bool zero_copy) {
    const uint16_t header_len = kHeaderLen;
    const uint16_t frame_len = static_cast<uint16_t>(header_len + (zero_copy ? 0u : len));
    if (frame_len > cfg_.mtu || frame_len > kMaxFrame) {
        return false;
    }
    if (pending_tx_key_rotation_ &&
        pending_tx_key_acknowledged_ &&
        (tx_nonce_counter_ + 1u) >= pending_tx_activate_nonce_) {
        tx_key_id_ = pending_tx_key_id_;
        pending_tx_key_rotation_ = false;
    }

    uint8_t* out = slot->frame;
    WriteU16(out + 0, kMagic);
    WriteU16(out + 2, seq);
    WriteU16(out + 4, has_recv_seq_ ? newest_recv_seq_ : 0);
    memset(out + 6, 0, kAckBitmapBytes);
    const uint32_t bits = has_recv_seq_ ? BuildAckBits(newest_recv_seq_) : 0;
    out[6] = static_cast<uint8_t>(bits & 0xFFu);
    out[7] = static_cast<uint8_t>((bits >> 8) & 0xFFu);
    out[8] = static_cast<uint8_t>((bits >> 16) & 0xFFu);
    out[9] = static_cast<uint8_t>((bits >> 24) & 0xFFu);
    WriteU32(out + 10, local_session_id_);
    const uint32_t nonce = ++tx_nonce_counter_;
    WriteU32(out + 14, nonce);
    WriteU16(out + 18, len);
    out[20] = 0;
    out[21] = tx_key_id_;
    WriteU64(out + 22, 0);
    if (!zero_copy) {
        memcpy(out + header_len, payload, len);
    }
    const uint64_t tag = ComputeAuthTag(out, kHeaderNoTagLen, payload, len, nonce, tx_key_id_);
    WriteU64(out + 22, tag);

    slot->seq = seq;
    slot->payload_len = len;
    slot->frame_len = frame_len;
    slot->zero_copy = zero_copy;
    slot->ext_payload = zero_copy ? payload : 0;
    slot->nonce = nonce;
    slot->auth_tag = tag;
    return true;
}

bool Endpoint::BuildControlFrame(uint8_t* out, uint16_t* out_len,
                                 uint8_t flags, uint32_t nonce,
                                 const uint8_t* payload, uint16_t payload_len) const {
    if (!out || !out_len) {
        return false;
    }
    if (payload_len > cfg_.max_payload) {
        return false;
    }
    WriteU16(out + 0, kMagic);
    WriteU16(out + 2, 0);
    WriteU16(out + 4, has_recv_seq_ ? newest_recv_seq_ : 0);
    const uint32_t bits = has_recv_seq_ ? BuildAckBits(newest_recv_seq_) : 0;
    out[6] = static_cast<uint8_t>(bits & 0xFFu);
    out[7] = static_cast<uint8_t>((bits >> 8) & 0xFFu);
    out[8] = static_cast<uint8_t>((bits >> 16) & 0xFFu);
    out[9] = static_cast<uint8_t>((bits >> 24) & 0xFFu);
    WriteU32(out + 10, local_session_id_);
    WriteU32(out + 14, nonce);
    WriteU16(out + 18, payload_len);
    out[20] = flags;
    out[21] = tx_key_id_;
    WriteU64(out + 22, 0);
    if (payload_len > 0) {
        memcpy(out + kHeaderLen, payload, payload_len);
    }
    const uint64_t tag = ComputeAuthTag(out, kHeaderNoTagLen, payload, payload_len, nonce, tx_key_id_);
    WriteU64(out + 22, tag);
    *out_len = static_cast<uint16_t>(kHeaderLen + payload_len);
    return true;
}

uint64_t Endpoint::ComputeAuthTag(const uint8_t* head_no_tag, uint16_t head_no_tag_len,
                                  const uint8_t* payload, uint16_t payload_len,
                                  uint32_t nonce, uint8_t key_id) const {
    if (!cfg_.enable_auth) {
        return 0;
    }
    uint64_t k0 = 0;
    uint64_t k1 = 0;
    if (!ResolveAuthKey(key_id, &k0, &k1)) {
        return 0;
    }
    uint8_t msg[kHeaderNoTagLen + kMaxFrame];
    uint16_t total = 0;
    memcpy(msg + total, head_no_tag, head_no_tag_len);
    total = static_cast<uint16_t>(total + head_no_tag_len);
    if (payload != 0 && payload_len > 0) {
        memcpy(msg + total, payload, payload_len);
        total = static_cast<uint16_t>(total + payload_len);
    }
    WriteU32(msg + 14, nonce);
    return SipHash24(msg, total, k0, k1);
}

bool Endpoint::ValidateAndTrackReplay(uint32_t nonce, bool* duplicate) {
    if (duplicate) {
        *duplicate = false;
    }
    if (rx_replay_max_nonce_ == 0) {
        rx_replay_max_nonce_ = nonce;
        rx_replay_bitmap_ = 1ull;
        return true;
    }
    if (nonce > rx_replay_max_nonce_) {
        const uint32_t d = nonce - rx_replay_max_nonce_;
        if (d >= 64u) {
            rx_replay_bitmap_ = 1ull;
        } else {
            rx_replay_bitmap_ = (rx_replay_bitmap_ << d) | 1ull;
        }
        rx_replay_max_nonce_ = nonce;
        return true;
    }
    const uint32_t d = rx_replay_max_nonce_ - nonce;
    if (d >= 64u) {
        return false;
    }
    const uint64_t mask = (1ull << d);
    if ((rx_replay_bitmap_ & mask) != 0ull) {
        if (duplicate) {
            *duplicate = true;
        }
        return false;
    }
    rx_replay_bitmap_ |= mask;
    return true;
}

bool Endpoint::ConsumePacingBudget(uint16_t bytes) {
    if (cfg_.pacing_bytes_per_tick == 0) {
        return true;
    }
    if (bytes > pacing_budget_bytes_) {
        ++stats_.tx_dropped_by_pacing;
        return false;
    }
    pacing_budget_bytes_ = static_cast<uint16_t>(pacing_budget_bytes_ - bytes);
    return true;
}

bool Endpoint::SendControl(uint8_t flags, uint32_t now_ms,
                           const uint8_t* payload, uint16_t payload_len) {
    if (pending_tx_key_rotation_ &&
        pending_tx_key_acknowledged_ &&
        (tx_nonce_counter_ + 1u) >= pending_tx_activate_nonce_) {
        tx_key_id_ = pending_tx_key_id_;
        pending_tx_key_rotation_ = false;
    }
    uint8_t frame[kHeaderLen + kMaxFrame];
    uint16_t frame_len = 0;
    const uint32_t nonce = ++tx_nonce_counter_;
    if (!BuildControlFrame(frame, &frame_len, flags, nonce, payload, payload_len)) {
        return false;
    }
    if (!ConsumePacingBudget(frame_len)) {
        return false;
    }
    if (!hooks_.send_raw(hooks_.user, frame, frame_len)) {
        return false;
    }
    last_tx_ms_ = now_ms;
    ++stats_.tx_packets;
    ++stats_.tx_control;
    return true;
}

bool Endpoint::SendFrame(PacketSlot* slot, uint32_t now_ms, bool is_retransmit) {
    uint16_t bytes_to_send = slot->frame_len;
    if (slot->zero_copy) {
        bytes_to_send = static_cast<uint16_t>(kHeaderLen + slot->payload_len);
    }
    if (!ConsumePacingBudget(bytes_to_send)) {
        return false;
    }
    bool ok = false;
    if (slot->zero_copy && hooks_.send_raw_vec != 0) {
        ok = hooks_.send_raw_vec(hooks_.user,
                                 slot->frame, kHeaderLen,
                                 slot->ext_payload, slot->payload_len);
    } else {
        if (slot->zero_copy) {
            if (static_cast<uint16_t>(kHeaderLen + slot->payload_len) > kMaxFrame) {
                return false;
            }
            memcpy(slot->frame + kHeaderLen, slot->ext_payload, slot->payload_len);
            slot->frame_len = static_cast<uint16_t>(kHeaderLen + slot->payload_len);
        }
        ok = hooks_.send_raw(hooks_.user, slot->frame, slot->frame_len);
    }
    if (!ok) {
        return false;
    }
    slot->last_sent_ms = now_ms;
    slot->ever_sent = true;
    last_tx_ms_ = now_ms;
    if (is_retransmit) {
        ++stats_.tx_retransmits;
    } else {
        ++stats_.tx_packets;
    }
    return true;
}

SendStatus Endpoint::Send(const uint8_t* payload, uint16_t len) {
    if (!initialized_) {
        return SendStatus::kQueueFull;
    }
    if (conn_state_ != ConnectionState::kConnected) {
        return SendStatus::kNotConnected;
    }
    if (len == 0 || len > cfg_.max_payload) {
        return SendStatus::kPayloadTooLarge;
    }

    uint16_t inflight = 0;
    for (uint16_t i = 0; i < kMaxQueue; ++i) {
        if (send_slots_[i].used && !send_slots_[i].acked) {
            ++inflight;
        }
    }
    if (inflight >= cfg_.send_window) {
        return SendStatus::kQueueFull;
    }

    PacketSlot* slot = AllocateSendSlot();
    if (!slot) {
        return SendStatus::kQueueFull;
    }

    const uint16_t seq = next_send_seq_;
    ++next_send_seq_;
    if (!BuildDataFrame(slot, seq, payload, len, false)) {
        slot->used = false;
        return SendStatus::kPayloadTooLarge;
    }

    const uint32_t now = hooks_.now_ms(hooks_.user);
    (void)SendFrame(slot, now, false);
    return SendStatus::kOk;
}

SendStatus Endpoint::SendZeroCopy(const uint8_t* payload, uint16_t len) {
    if (!initialized_) {
        return SendStatus::kQueueFull;
    }
    if (conn_state_ != ConnectionState::kConnected) {
        return SendStatus::kNotConnected;
    }
    if (hooks_.send_raw_vec == 0) {
        return SendStatus::kUnsupported;
    }
    if (len == 0 || len > cfg_.max_payload) {
        return SendStatus::kPayloadTooLarge;
    }

    uint16_t inflight = 0;
    for (uint16_t i = 0; i < kMaxQueue; ++i) {
        if (send_slots_[i].used && !send_slots_[i].acked) {
            ++inflight;
        }
    }
    if (inflight >= cfg_.send_window) {
        return SendStatus::kQueueFull;
    }

    PacketSlot* slot = AllocateSendSlot();
    if (!slot) {
        return SendStatus::kQueueFull;
    }

    const uint16_t seq = next_send_seq_;
    ++next_send_seq_;
    if (!BuildDataFrame(slot, seq, payload, len, true)) {
        slot->used = false;
        return SendStatus::kPayloadTooLarge;
    }

    const uint32_t now = hooks_.now_ms(hooks_.user);
    (void)SendFrame(slot, now, false);
    return SendStatus::kOk;
}

uint32_t Endpoint::BuildAckBits(uint16_t newest) const {
    uint32_t bits = 0;
    for (uint8_t i = 0; i < 32; ++i) {
        const uint16_t seq = static_cast<uint16_t>(newest - (i + 1u));
        for (uint16_t j = 0; j < kMaxQueue; ++j) {
            if (recv_slots_[j].used && recv_slots_[j].seq == seq) {
                bits |= (1u << i);
                break;
            }
        }
    }
    return bits;
}

void Endpoint::HandleAck(uint16_t ack, uint32_t ack_bits) {
    bool any_new_acked = false;
    for (uint16_t i = 0; i < kMaxQueue; ++i) {
        if (!send_slots_[i].used || send_slots_[i].acked) {
            continue;
        }
        PacketSlot* slot = &send_slots_[i];
        bool now_acked = false;
        if (slot->seq == ack) {
            now_acked = true;
        } else if (IsSeqLess(slot->seq, ack)) {
            const uint16_t d = DistanceFrom(slot->seq, ack);
            if (d >= 1 && d <= 32) {
                const uint32_t mask = 1u << (d - 1u);
                now_acked = ((ack_bits & mask) != 0u);
            }
        }
        if (now_acked) {
            const uint32_t now = hooks_.now_ms(hooks_.user);
            if (slot->ever_sent && slot->last_sent_ms > 0) {
                const uint32_t sample = now - slot->last_sent_ms;
                MaybeAdjustRto(sample);
            }
            slot->acked = true;
            slot->used = false;
            ++stats_.tx_acks;
            any_new_acked = true;
        }
    }

    if (any_new_acked) {
        last_seen_ack_ = ack;
        last_seen_ack_bits_ = ack_bits;
        dup_ack_count_ = 0;
        return;
    }

    if (ack == last_seen_ack_ && ack_bits == last_seen_ack_bits_) {
        if (dup_ack_count_ < 255) {
            ++dup_ack_count_;
        }
        if (dup_ack_count_ >= 3) {
            const uint32_t now = hooks_.now_ms(hooks_.user);
            FastRetransmit(now);
            dup_ack_count_ = 0;
        }
    } else {
        last_seen_ack_ = ack;
        last_seen_ack_bits_ = ack_bits;
        dup_ack_count_ = 0;
    }
}

void Endpoint::FastRetransmit(uint32_t now_ms) {
    PacketSlot* victim = 0;
    for (uint16_t i = 0; i < kMaxQueue; ++i) {
        PacketSlot* slot = &send_slots_[i];
        if (!slot->used || slot->acked || !slot->ever_sent) {
            continue;
        }
        if (!victim || IsSeqLess(slot->seq, victim->seq)) {
            victim = slot;
        }
    }
    if (!victim) {
        return;
    }
    if (victim->retries >= cfg_.max_retransmits) {
        victim->used = false;
        return;
    }
    ++victim->retries;
    (void)SendFrame(victim, now_ms, true);
}

void Endpoint::HandleControl(uint8_t flags, uint32_t sender_session_id, uint32_t now_ms,
                             const uint8_t* payload, uint16_t payload_len) {
    if ((flags & kFlagSyn) != 0u) {
        if (!has_peer_session_id_ || peer_session_id_ != sender_session_id) {
            rx_replay_max_nonce_ = 0;
            rx_replay_bitmap_ = 0;
            peer_key_rotation_known_ = false;
        }
        peer_session_id_ = sender_session_id;
        has_peer_session_id_ = true;
        if (conn_state_ != ConnectionState::kConnected) {
            conn_state_ = ConnectionState::kConnected;
            connect_retries_ = 0;
            ++stats_.connect_success;
        }
        (void)SendControl(static_cast<uint8_t>(kFlagAckOnly | kFlagSynAck), now_ms, 0, 0);
    }
    if ((flags & kFlagSynAck) != 0u) {
        if (conn_state_ == ConnectionState::kConnecting) {
            if (!has_peer_session_id_ || peer_session_id_ != sender_session_id) {
                rx_replay_max_nonce_ = 0;
                rx_replay_bitmap_ = 0;
                peer_key_rotation_known_ = false;
            }
            peer_session_id_ = sender_session_id;
            has_peer_session_id_ = true;
            conn_state_ = ConnectionState::kConnected;
            connect_retries_ = 0;
            ++stats_.connect_success;
            (void)SendControl(kFlagAckOnly, now_ms, 0, 0);
        }
    }
    if ((flags & kFlagPing) != 0u) {
        (void)SendControl(kFlagAckOnly, now_ms, 0, 0);
    }
    if ((flags & kFlagKeyUpdate) != 0u) {
        if (payload_len == 5 && payload != 0) {
            const uint8_t next_key_id = payload[0];
            const uint32_t activate_nonce = ReadU32(payload + 1);
            uint64_t k0 = 0;
            uint64_t k1 = 0;
            if (ResolveAuthKey(next_key_id, &k0, &k1) && activate_nonce > 0) {
                peer_key_rotation_known_ = true;
                peer_next_key_id_ = next_key_id;
                peer_key_activate_nonce_ = activate_nonce;
                uint8_t ack_payload[5];
                ack_payload[0] = next_key_id;
                WriteU32(ack_payload + 1, activate_nonce);
                (void)SendControl(static_cast<uint8_t>(kFlagAckOnly | kFlagKeyUpdateAck), now_ms, ack_payload, 5);
            }
        }
        (void)SendControl(kFlagAckOnly, now_ms, 0, 0);
    }
    if ((flags & kFlagKeyUpdateAck) != 0u) {
        if (payload_len == 5 && payload != 0) {
            const uint8_t ack_key_id = payload[0];
            const uint32_t ack_activate_nonce = ReadU32(payload + 1);
            if (pending_tx_key_rotation_ &&
                ack_key_id == pending_tx_key_id_ &&
                ack_activate_nonce == pending_tx_activate_nonce_) {
                pending_tx_key_acknowledged_ = true;
                pending_tx_key_retry_count_ = 0;
            }
        }
    }
}

void Endpoint::InsertRecv(uint16_t seq, const uint8_t* payload, uint16_t len) {
    if (!SeqInWindow(seq, recv_next_seq_, cfg_.recv_window)) {
        ++stats_.rx_dropped;
        return;
    }

    RecvSlot* existing = FindRecvSlotBySeq(seq);
    if (existing) {
        ++stats_.rx_duplicates;
        return;
    }

    RecvSlot* slot = AllocateRecvSlot();
    if (!slot) {
        ++stats_.rx_dropped;
        return;
    }

    slot->seq = seq;
    slot->len = len;
    memcpy(slot->data, payload, len);
}

void Endpoint::DrainInOrder() {
    for (;;) {
        RecvSlot* slot = FindRecvSlotBySeq(recv_next_seq_);
        if (!slot) {
            break;
        }
        hooks_.on_deliver(hooks_.user, slot->data, slot->len);
        slot->used = false;
        ++recv_next_seq_;
        ++stats_.rx_delivered;
    }
}

void Endpoint::OnUdpPacket(const uint8_t* data, uint16_t len) {
    if (!initialized_) {
        return;
    }
    const uint16_t header_len = kHeaderLen;
    if (len < header_len) {
        ++stats_.rx_dropped;
        return;
    }
    if (ReadU16(data + 0) != kMagic) {
        ++stats_.rx_dropped;
        return;
    }

    ++stats_.rx_packets;
    last_rx_ms_ = hooks_.now_ms(hooks_.user);
    const uint16_t seq = ReadU16(data + 2);
    const uint16_t ack = ReadU16(data + 4);
    const uint32_t ack_bits = static_cast<uint32_t>(data[6]) |
                              (static_cast<uint32_t>(data[7]) << 8) |
                              (static_cast<uint32_t>(data[8]) << 16) |
                              (static_cast<uint32_t>(data[9]) << 24);
    const uint32_t session_id = ReadU32(data + 10);
    const uint32_t nonce = ReadU32(data + 14);
    const uint16_t payload_len = ReadU16(data + 18);
    const uint8_t flags = data[20];
    const uint8_t key_id = data[21];
    const uint64_t auth_tag = ReadU64(data + 22);

    if (payload_len > cfg_.max_payload || static_cast<uint16_t>(header_len + payload_len) != len) {
        ++stats_.rx_dropped;
        return;
    }
    if (session_id == 0) {
        ++stats_.rx_dropped;
        return;
    }
    if (nonce == 0) {
        ++stats_.rx_dropped;
        return;
    }
    if (peer_key_rotation_known_) {
        if (nonce < peer_key_activate_nonce_ && key_id == peer_next_key_id_) {
            ++stats_.rx_dropped;
            return;
        }
        if (nonce >= peer_key_activate_nonce_ && key_id != peer_next_key_id_) {
            ++stats_.rx_dropped;
            return;
        }
    }

    const uint8_t* payload = data + header_len;
    if (cfg_.enable_auth) {
        const uint64_t expected = ComputeAuthTag(data, kHeaderNoTagLen, payload, payload_len, nonce, key_id);
        if (expected == 0) {
            ++stats_.rx_dropped;
            return;
        }
        if (expected != auth_tag) {
            ++stats_.rx_dropped;
            return;
        }
    }
    if (has_peer_session_id_ && session_id != peer_session_id_) {
        ++stats_.rx_dropped;
        return;
    }

    HandleAck(ack, ack_bits);
    bool replay_dup = false;
    if (!ValidateAndTrackReplay(nonce, &replay_dup)) {
        if (replay_dup) {
            ++stats_.rx_duplicates;
            if ((flags & kFlagAckOnly) == 0u) {
                ack_dirty_ = true;
            }
        } else {
            ++stats_.rx_dropped;
        }
        return;
    }
    if (flags != 0) {
        ++stats_.rx_control;
        HandleControl(flags, session_id, last_rx_ms_, payload, payload_len);
    }

    if ((flags & kFlagAckOnly) != 0u) {
        return;
    }
    if (payload_len == 0) {
        return;
    }
    if (conn_state_ != ConnectionState::kConnected) {
        ++stats_.rx_dropped;
        return;
    }

    InsertRecv(seq, payload, payload_len);

    if (!has_recv_seq_ || IsSeqLess(newest_recv_seq_, seq)) {
        newest_recv_seq_ = seq;
        has_recv_seq_ = true;
    }
    ack_dirty_ = true;

    DrainInOrder();
}

void Endpoint::FlushAcks(uint32_t now_ms, bool force) {
    if (conn_state_ != ConnectionState::kConnected || !ack_dirty_ || !has_recv_seq_) {
        return;
    }
    if (!force && (now_ms - last_ack_flush_ms_) < cfg_.ack_delay_ms) {
        return;
    }
    if (!force) {
        bool has_pending_data = false;
        for (uint16_t i = 0; i < kMaxQueue; ++i) {
            if (send_slots_[i].used && !send_slots_[i].acked) {
                has_pending_data = true;
                break;
            }
        }
        if (has_pending_data && (now_ms - last_ack_flush_ms_) < (cfg_.ack_delay_ms * 2u)) {
            return;
        }
    }

    if (SendControl(kFlagAckOnly, now_ms, 0, 0)) {
        last_ack_flush_ms_ = now_ms;
        ack_dirty_ = false;
    }
}

void Endpoint::MaybeAdjustRto(uint32_t sample_ms) {
    if (sample_ms == 0) {
        sample_ms = 1;
    }
    if (smoothed_rtt_ == 0) {
        smoothed_rtt_ = sample_ms;
        rtt_var_ = sample_ms / 2;
    } else {
        const int32_t err = static_cast<int32_t>(sample_ms) - static_cast<int32_t>(smoothed_rtt_);
        smoothed_rtt_ = static_cast<uint32_t>(static_cast<int32_t>(smoothed_rtt_) + err / 8);
        const int32_t abs_err = (err < 0) ? -err : err;
        rtt_var_ = static_cast<uint32_t>(static_cast<int32_t>(rtt_var_) + (abs_err - static_cast<int32_t>(rtt_var_)) / 4);
    }
    uint32_t rto = smoothed_rtt_ + (rtt_var_ * 4u);
    if (rto < cfg_.retransmit_min_ms) {
        rto = cfg_.retransmit_min_ms;
    }
    if (rto > cfg_.retransmit_max_ms) {
        rto = cfg_.retransmit_max_ms;
    }
    rto_ms_ = rto;
}

void Endpoint::Tick() {
    if (!initialized_) {
        return;
    }
    const uint32_t now = hooks_.now_ms(hooks_.user);
    if (pending_tx_key_rotation_ &&
        pending_tx_key_acknowledged_ &&
        (tx_nonce_counter_ + 1u) >= pending_tx_activate_nonce_) {
        tx_key_id_ = pending_tx_key_id_;
        pending_tx_key_rotation_ = false;
    }
    if (!pending_tx_key_rotation_ && pending_tx_key_acknowledged_ && pending_tx_old_key_id_ != 0) {
        for (uint8_t i = 0; i < 2; ++i) {
            if (auth_keys_[i].used &&
                auth_keys_[i].key_id == pending_tx_old_key_id_ &&
                auth_keys_[i].key_id != tx_key_id_) {
                auth_keys_[i].used = false;
            }
        }
        pending_tx_old_key_id_ = 0;
        pending_tx_key_acknowledged_ = false;
    }
    if (cfg_.pacing_bytes_per_tick > 0) {
        uint32_t budget = static_cast<uint32_t>(pacing_budget_bytes_) + cfg_.pacing_bytes_per_tick;
        const uint32_t max_budget = static_cast<uint32_t>(cfg_.mtu) * static_cast<uint32_t>(cfg_.send_window);
        if (budget > max_budget) {
            budget = max_budget;
        }
        pacing_budget_bytes_ = static_cast<uint16_t>(budget);
    }

    if (conn_state_ == ConnectionState::kConnecting) {
        if ((now - last_connect_try_ms_) >= cfg_.connect_retry_ms) {
            if (connect_retries_ >= cfg_.max_connect_retries) {
                conn_state_ = ConnectionState::kTimedOut;
                ++stats_.connect_timeouts;
            } else {
                ++connect_retries_;
                ++stats_.connect_attempts;
                last_connect_try_ms_ = now;
                (void)SendControl(kFlagSyn, now, 0, 0);
            }
        }
    }

    if (conn_state_ == ConnectionState::kConnected) {
        if (pending_tx_key_rotation_) {
            if (!pending_tx_key_acknowledged_ && (tx_nonce_counter_ + 1u) >= pending_tx_activate_nonce_) {
                pending_tx_activate_nonce_ = (tx_nonce_counter_ + 1u) + 8u;
                last_key_update_announce_ms_ = 0;
            }
            const uint16_t retry_ms = (cfg_.key_update_retry_ms > 0) ? cfg_.key_update_retry_ms : cfg_.connect_retry_ms;
            const bool need_announce = !pending_tx_key_acknowledged_ &&
                                       ((last_key_update_announce_ms_ == 0) ||
                                        ((now - last_key_update_announce_ms_) >= retry_ms));
            if (need_announce) {
                uint8_t ctrl[5];
                ctrl[0] = pending_tx_key_id_;
                WriteU32(ctrl + 1, pending_tx_activate_nonce_);
                if (SendControl(static_cast<uint8_t>(kFlagAckOnly | kFlagKeyUpdate), now, ctrl, 5)) {
                    last_key_update_announce_ms_ = now;
                    if (pending_tx_key_retry_count_ < 255) {
                        ++pending_tx_key_retry_count_;
                    }
                    if (cfg_.key_update_max_retries > 0 &&
                        pending_tx_key_retry_count_ > cfg_.key_update_max_retries) {
                        pending_tx_key_rotation_ = false;
                        pending_tx_key_id_ = 0;
                        pending_tx_activate_nonce_ = 0;
                        pending_tx_key_acknowledged_ = false;
                        pending_tx_old_key_id_ = 0;
                        pending_tx_key_retry_count_ = 0;
                    }
                }
            }
        }
        if (cfg_.idle_timeout_ms > 0 && (now - last_rx_ms_) >= cfg_.idle_timeout_ms) {
            conn_state_ = ConnectionState::kTimedOut;
            ++stats_.connect_timeouts;
            return;
        }
        if (cfg_.heartbeat_ms > 0 && (now - last_tx_ms_) >= cfg_.heartbeat_ms) {
            (void)SendControl(static_cast<uint8_t>(kFlagAckOnly | kFlagPing), now, 0, 0);
        }
    }

    for (uint16_t i = 0; i < kMaxQueue; ++i) {
        PacketSlot* slot = &send_slots_[i];
        if (!slot->used || slot->acked) {
            continue;
        }
        if (!slot->ever_sent) {
            (void)SendFrame(slot, now, false);
            continue;
        }
        if ((now - slot->last_sent_ms) < rto_ms_) {
            continue;
        }
        if (slot->retries >= cfg_.max_retransmits) {
            slot->used = false;
            continue;
        }
        ++slot->retries;
        (void)SendFrame(slot, now, true);
    }

    FlushAcks(now, false);
}

uint16_t Endpoint::GetPendingSend() const {
    uint16_t n = 0;
    for (uint16_t i = 0; i < kMaxQueue; ++i) {
        if (send_slots_[i].used && !send_slots_[i].acked) {
            ++n;
        }
    }
    return n;
}

}  // namespace rudp
