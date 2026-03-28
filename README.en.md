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

## Tuning profiles

- `ConfigForProfile(kBalanced)`: default balanced profile.
- `ConfigForProfile(kLowLatency)`: lower ack delay and retransmit baseline.
- `ConfigForProfile(kLowPower)`: lower wake frequency and wider timeout windows.
- `ConfigForProfile(kLossyLink)`: stronger retry tolerance for unstable links.

## Layout

- `include/rudp/rudp.hpp`: public API
- `include/rudp/manager.hpp`: multi-connection manager API
- `src/rudp.cpp`: protocol implementation
- `src/manager.cpp`: connection routing/session manager
- `tests/self_test.cpp`: connect/queue/key-rotation/timeout smoke test
- `tests/reliability_test.cpp`: deterministic loss/reorder regression test

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

## Install and consume

```bash
cmake -S . -B build -DRUDP_INSTALL=ON -DRUDP_BUILD_TESTS=OFF
cmake --build build --config Release
cmake --install build --config Release
```

- CMake package: `rudpConfig.cmake` + `rudpTargets.cmake`
- pkg-config: `rudp.pc`
- Package metadata: `vcpkg.json`, `conanfile.py`

## Run self test

```bash
build/rudp_self_test
build/rudp_reliability_test
build/rudp_bench
build/rudp_manager_test
```

## Quick examples

```bash
build/rudp_example_endpoint
build/rudp_example_manager
```

Example sources:

- `examples/basic_endpoint.cpp`
- `examples/basic_manager.cpp`
- `examples/socket_posix.cpp` (POSIX UDP socket)
- `examples/socket_winsock.cpp` (WinSock UDP socket)
- `examples/stm32/stm32_f4_udp.cpp` (STM32 F4 reference template)
- `examples/esp32/esp32_udp.cpp` (ESP32 reference template)
- `examples/raspberry_pi/raspberry_pi_udp.cpp` (Raspberry Pi POSIX example)
- `examples/arduino/arduino_udp/arduino_udp.ino` (Arduino reference template)

Note: embedded examples are aligned with the current API, but they are still reference templates and are not built in the main CI pipeline.

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
11. Metrics export: `Endpoint::GetRuntimeMetrics()` for queue/RTO/RTT/retransmit/drop ratios.

## Manager API quick view

1. `rudp::ConnectionManager::Init(config, hooks)` initializes multi-connection routing.
2. `Open(key, start_connect)` creates/finds a keyed endpoint; `Remove(key)` closes it.
3. `OnUdpPacket(key, data, len)` routes inbound datagrams by session key.
4. `Send/SendZeroCopy/Tick` work at manager level per key.
5. Prefer `IsConnected/GetConnectionState/GetStats/GetRuntimeMetrics` for concurrent-safe reads.
6. `Find()` is a legacy helper; returned `Endpoint*` is not stable across concurrent `Remove/Open`.
