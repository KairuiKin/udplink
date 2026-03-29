# udplink

Reliable UDP library for embedded and MCU-class targets (C++11), with fixed-memory protocol state, pluggable I/O hooks, and a narrow but usable installed-package surface for both C++ and C consumers.

## Current status

- Core C++ build/test paths work on Windows, Linux, and macOS.
- The minimal C ABI is implemented in `include/rudp/rudp_c.h` / `src/rudp_c.cpp`.
- Install/export metadata and installed-package consumer paths are in place.
- Windows is currently the strongest validation path in this repository: `ctest`, install-consume, the maintained C consumer example, and release-style local checks have all been exercised.
- PlatformIO remains an Arduino-first narrow path, not a broad embedded support promise.
- The current C ABI is implemented but not yet declared long-term stable; the repository's current recommendation remains `wait`.

## Documentation Entry Points

- Public landing page: https://kairuikin.github.io/udplink/
- Rendered docs browser: https://kairuikin.github.io/udplink/guide.html
- Current project status: `docs/project-status.md`
- C API quickstart: `docs/c-api-quickstart.md`
- Arduino Mega W5100 bring-up: `docs/arduino-mega-w5100-bringup.md`

## Goals

- Reliable, ordered delivery on top of UDP without full TCP complexity.
- MCU-friendly operation: fixed memory, no protocol hot-path heap allocation, no thread requirement.
- Production-oriented integration via pluggable hooks, configurable retransmission/window behavior, and runtime stats.

## Core design

- Protocol core is decoupled from socket I/O using callbacks:
  - current time (`now_ms`)
  - raw UDP send (`send_raw`)
  - payload delivery (`on_deliver`)
- Optional concurrency guard via `enter_critical/leave_critical` callbacks.
- Selective ACK: cumulative ACK + 32-bit bitmap for out-of-order recovery.
- Adaptive RTO using RTT + variance estimation.
- Static TX/RX slot arrays for predictable CPU and memory costs.
- Connection lifecycle: `SYN/SYN-ACK` handshake, heartbeat, idle timeout.
- Optional pacing with `pacing_bytes_per_tick`.
- Session isolation with `session_id`.
- Nonce-based anti-replay window.
- Optional authentication (SipHash-2-4) and online key rotation.
- Zero-copy send path via `SendZeroCopy` + `send_raw_vec`.

## Layout

- `include/rudp/rudp.hpp`: C++ endpoint API
- `include/rudp/manager.hpp`: multi-connection manager API
- `include/rudp/rudp_c.h`: minimal C ABI header
- `src/rudp.cpp`: protocol implementation
- `src/manager.cpp`: connection manager implementation
- `src/rudp_c.cpp`: C ABI wrapper implementation
- `tests/`: smoke/reliability/manager/C ABI/install-consume checks
- `examples/`: C++, C, and embedded reference examples
- `docs/`: ABI decisions, release flow, board bring-up, and project status

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

## Test

### Unix-like

```bash
ctest --test-dir build --output-on-failure
```

### Windows PowerShell

```powershell
ctest --test-dir build -C Release --output-on-failure
```

The repository currently treats the following as the practical Windows completion bar:

1. the main `Release` build succeeds
2. `ctest --test-dir build -C Release --output-on-failure` passes
3. installed-package consumers configure, build, and run successfully

## Install and consume

```bash
cmake -S . -B build-install -DRUDP_INSTALL=ON -DRUDP_BUILD_TESTS=OFF -DRUDP_BUILD_EXAMPLES=OFF
cmake --build build-install --config Release
cmake --install build-install --config Release --prefix ./stage
```

Installed output includes:

- `rudpConfig.cmake` / `rudpTargets.cmake`
- `rudp.pc`
- public headers, including `include/rudp/rudp_c.h`

## Installed-package consumer checks

### Unix-like

```bash
cmake -S tests/install_consume -B build-consume -DCMAKE_PREFIX_PATH=./stage
cmake -S tests/install_consume_c -B build-consume-c -DCMAKE_PREFIX_PATH=./stage
cmake -S examples/c_api/install_consume -B build-example-c-api -DCMAKE_PREFIX_PATH=./stage
cmake --build build-consume --config Release
cmake --build build-consume-c --config Release
cmake --build build-example-c-api --config Release
```

### Windows PowerShell

```powershell
cmake -S tests/install_consume -B build-consume -Drudp_DIR:PATH="$PWD/stage/lib/cmake/rudp"
cmake -S tests/install_consume_c -B build-consume-c -Drudp_DIR:PATH="$PWD/stage/lib/cmake/rudp"
cmake -S examples/c_api/install_consume -B build-example-c-api -Drudp_DIR:PATH="$PWD/stage/lib/cmake/rudp"
cmake --build build-consume --config Release
cmake --build build-consume-c --config Release
cmake --build build-example-c-api --config Release
```

## Examples

Desktop examples:

- `examples/basic_endpoint.cpp`
- `examples/basic_manager.cpp`
- `examples/socket_posix.cpp`
- `examples/socket_winsock.cpp`
- `examples/socket_peer.cpp`

Embedded reference templates:

- `examples/stm32/stm32_f4_udp.cpp`
- `examples/esp32/esp32_udp.cpp`
- `examples/raspberry_pi/raspberry_pi_udp.cpp`
- `examples/arduino/arduino_udp/arduino_udp.ino`

Maintained C installed-package example:

- `examples/c_api/install_consume/main.c`

Embedded examples are reference templates, not broad CI-backed platform guarantees.
The first narrow board-backed route is documented in `docs/arduino-mega-w5100-bringup.md`.

For release-style local validation, use:

```bash
python scripts/release_check.py
```

## C ABI position

The key repository position today is:

- the C ABI is implemented
- the C ABI is exercised by tests and installed-package consumers
- the C ABI is not yet declared long-term stable
- `examples/c_api/install_consume/` is strong evidence, but not yet an officially blessed downstream baseline

See also:

- `docs/c-api-quickstart.md`
- `docs/c-abi-compatibility.md`
- `docs/c-abi-stability-gate.md`
- `docs/c-abi-stability-assessment.md`
- `docs/c-abi-downstream-baseline-decision.md`
- `docs/c-abi-stability-decision-playbook.md`

## API quick view

1. `rudp::Endpoint::Init(config, hooks)` initializes an endpoint.
2. `rudp::Endpoint::StartConnect()` starts active handshake.
3. `rudp::Endpoint::Send(data, len)` sends reliable messages once connected.
4. `rudp::Endpoint::SendZeroCopy(data, len)` avoids payload copy when `send_raw_vec` is available.
5. Feed each inbound UDP datagram through `OnUdpPacket(data, len)`.
6. Call `Tick()` periodically for retransmit, heartbeat, pacing, and ACK flush.
7. Use `Endpoint::GetRuntimeMetrics()` for queue/RTO/RTT/retransmit/drop metrics.

## Manager API quick view

1. `rudp::ConnectionManager::Init(config, hooks)` initializes multi-connection routing.
2. `Open(key, start_connect)` creates/finds a keyed endpoint and `Remove(key)` closes it.
3. `OnUdpPacket(key, data, len)` routes inbound datagrams by session key.
4. Prefer manager-level send/query APIs for new code.
5. `Endpoint*` returned by `Open()/Find()` remains a compatibility escape hatch, not the preferred long-term direction.

## C API quick view

1. `#include <rudp/rudp_c.h>` to use the minimal C ABI surface.
2. Initialize config with `rudp_default_config_v1()` or `rudp_config_for_profile_v1()`.
3. Call `rudp_endpoint_create()` before `rudp_endpoint_init()`.
4. Drive handshake and timers via `rudp_endpoint_start_connect()` and `rudp_endpoint_tick()`.
5. Feed inbound UDP datagrams through `rudp_endpoint_on_udp_packet()`.
6. Send data with `rudp_endpoint_send()`.
7. Query state and metrics with the public C getters.
