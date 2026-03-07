# udplink

Reliable UDP library for embedded and MCU-class targets (C++11).

## Goals

- Reliable, ordered delivery on top of UDP without full TCP complexity.
- MCU friendly: fixed memory, no dynamic allocation, no thread requirement.
- Production-oriented: pluggable network hooks, configurable retransmission/window, runtime stats.

## Core design

- Protocol core is decoupled from socket I/O using callbacks:
  - current time (`now_ms`)
  - raw UDP send (`send_raw`)
  - payload delivery (`on_deliver`)
- Optional concurrency guard via `enter_critical/leave_critical` callbacks.
- Selective ACK: cumulative ACK + 32-bit bitmap for out-of-order recovery.
- Adaptive RTO using RTT + variance estimator.
- Static slot arrays for TX/RX to keep predictable CPU and memory costs.
- Connection lifecycle: `SYN/SYN-ACK` handshake, heartbeat, idle timeout.
- Optional pacing with `pacing_bytes_per_tick` to control burst pressure on MCU.
- Session isolation with per-connection `session_id` to prevent packet mix-up.
- Nonce-based anti-replay sliding window (64 packets).
- Optional authentication (SipHash-2-4) with `enable_auth` + `auth_key0/auth_key1` (legacy `auth_psk` fallback).
- Online key rotation with on-wire `key_id` and dual-key verification window.
- Fast retransmit on duplicate ACK patterns to reduce recovery latency.
- Zero-copy send path via `SendZeroCopy` + `send_raw_vec`.

## Layout

- `include/rudp/rudp.hpp`: public API
- `src/rudp.cpp`: protocol implementation
- `tests/self_test.cpp`: packet-loss/reorder simulation test

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

## Run self test

```bash
build/rudp_self_test
build/rudp_bench
```

On Windows, executable is `rudp_self_test.exe`.

## API quick view

1. `rudp::Endpoint::Init(config, hooks)` initializes endpoint.
2. `rudp::Endpoint::StartConnect()` starts active handshake.
3. `rudp::Endpoint::Send(data, len)` sends reliable messages when `IsConnected()==true`.
4. `rudp::Endpoint::SendZeroCopy(data, len)` sends payload without payload-copy (needs `send_raw_vec`).
5. Call `OnUdpPacket(data, len)` for each UDP datagram.
6. Call `Tick()` periodically for retransmit, heartbeat and ACK flush.
7. Manual switch: `SetAuthKey(new_id, k0, k1, false)` then `RotateTxKey(new_id)`.
8. Automatic switch: `ScheduleTxKeyRotation(new_id, lead_packets)` with activation point control frame.
9. Protocol sends `KEY_UPDATE_ACK`; sender retires old key after acknowledgement.
10. On `KEY_UPDATE_ACK` timeout, sender postpones activation and retries; rotation is canceled after max retries.
