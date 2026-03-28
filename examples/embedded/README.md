# Embedded Examples

This directory contains reference templates for integrating `udplink` on embedded targets.

## Scope

- These examples are aligned with the current `rudp::Hooks` / `Endpoint::StartConnect()` API.
- They are not built in the main CI pipeline.
- Treat them as integration templates: keep the udplink calls, replace the board-specific UDP and timer hooks.

## Platforms

### STM32 (`examples/stm32/`)

- Target: STM32F4xx with lwIP, W5500, or another UDP-capable transport.
- Profile: `ConfigProfile::kLowPower` by default.
- Work to finish locally: timer source, UDP send path, UDP poll path.

### ESP32 (`examples/esp32/`)

- Target: ESP-IDF + lwIP sockets.
- Profile: `ConfigProfile::kLowPower` by default.
- Work to finish locally: network bring-up, peer address, task sizing.

### Arduino (`examples/arduino/arduino_udp/`)

- Target: Arduino Ethernet-based boards.
- Profile: `ConfigProfile::kLowPower` by default.
- Work to finish locally: board/library selection, IP layout, memory trimming.

### Raspberry Pi (`examples/raspberry_pi/`)

- Target: Raspberry Pi OS / Linux.
- Status: standalone POSIX example with current API.

## Notes

- See `examples/socket_posix.cpp` and `examples/socket_winsock.cpp` for the CI-covered desktop path.
- See `include/rudp/rudp.hpp` for the source of truth on public API.
- See `docs/api-mapping.md` for the current callback and method mapping used by examples.
