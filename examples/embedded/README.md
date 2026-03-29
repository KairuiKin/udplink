# Embedded Examples

This directory contains reference templates for integrating `udplink` on embedded targets.

## Scope

- These examples are aligned with the current `rudp::Hooks` / `Endpoint::StartConnect()` API.
- They are not built in the main CI pipeline.
- Treat them as integration templates: keep the udplink calls, replace the board-specific UDP and timer hooks.
- STM32 and ESP32 templates now also have a host-side compile-only validation path in CI.

## Platform Matrix

### STM32 (`examples/stm32/`)

- Target: STM32F4xx with lwIP, W5500, or another UDP-capable transport.
- Profile: `ConfigProfile::kLowPower` by default.
- Primary local work: timer source, UDP send path, UDP poll path.
- Platform notes: `examples/stm32/README.md`
- CI scope: compile-only template validation, not board-runtime validation.

### ESP32 (`examples/esp32/`)

- Target: ESP-IDF + lwIP sockets.
- Profile: `ConfigProfile::kLowPower` by default.
- Primary local work: network bring-up, peer address, task sizing.
- Platform notes: `examples/esp32/README.md`
- CI scope: compile-only template validation via `RUDP_TEMPLATE_COMPILE_ONLY` stubs.

### Arduino (`examples/arduino/arduino_udp/`)

- Target: Arduino Ethernet-based boards.
- Profile: `ConfigProfile::kLowPower` by default.
- Primary local work: board/library selection, IP layout, memory trimming.
- Platform notes: `examples/arduino/arduino_udp/README.md`

### Raspberry Pi (`examples/raspberry_pi/`)

- Target: Raspberry Pi OS / Linux.
- Status: standalone POSIX example with current API.
- Platform notes: `examples/raspberry_pi/README.md`

## Common Adaptation Checklist

- choose a timer source that returns milliseconds for `Hooks::now_ms`
- implement a non-blocking UDP send path for `Hooks::send_raw`
- add `send_raw_vec` only if you want zero-copy send coverage
- feed every inbound UDP datagram into `Endpoint::OnUdpPacket(...)`
- call `Endpoint::Tick()` at a stable cadence
- keep peer IP/port in the transport layer, not in `udplink` itself
- confirm the example still uses only public APIs from `include/rudp/rudp.hpp`

## Local Bring-up Sequence

1. Build or copy the template into your platform project.
2. Replace placeholder transport/timer hooks with platform code.
3. Configure local port, peer IP, and peer port.
4. Confirm `StartConnect()` is called on the active side.
5. Verify the connection reaches `IsConnected() == true`.
6. Send one small payload and confirm `OnDeliver` fires on the peer.

## Notes

- See `examples/socket_posix.cpp` and `examples/socket_winsock.cpp` for the CI-covered desktop path.
- See `include/rudp/rudp.hpp` for the source of truth on public API.
- See `docs/api-mapping.md` for the current callback and method mapping used by examples.

## Recommended First Board Path

- Start with `docs/arduino-mega-w5100-bringup.md` if you want the narrowest board-backed bring-up route currently documented in the repository.
- Treat other embedded examples as templates unless they gain an equally explicit bring-up document.
