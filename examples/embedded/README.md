# Embedded Examples

This directory contains examples for embedded platforms.

## Platforms

### STM32 (examples/stm32/)

Example for STM32F4xx MCU with Ethernet (W5500) or WiFi.

**Hardware:**
- STM32F4xx (F407, F429, etc.)
- Ethernet: W5500 SPI Ethernet module
- Or WiFi: ESP8266/ESP32 (as co-processor)

**Features:**
- Fixed memory allocation (no heap in core)
- Low power profile optimized
- Auth enabled by default

**Usage:**
1. Copy `stm32_f4_udp.cpp` to your STM32 project
2. Implement `Hardware_Init()` for your board
3. Configure network stack (LWIP)
4. Adjust `kLocalPort`, `kRemotePort` as needed

---

### ESP32 (examples/esp32/)

Example for Espressif ESP32 with ESP-IDF.

**Hardware:**
- ESP32 / ESP32-S2 / ESP32-S3 / ESP32-C3
- Ethernet (LAN8720) or WiFi

**Features:**
- FreeRTOS integration
- lwIP socket API
- Low latency profile

**Usage:**
1. Add to your ESP-IDF project
2. Configure component.cmake to include udplink
3. Update IP address in `kRemotePort`
4. `idf.py build flash monitor`

---

## Memory Requirements

| Platform | RAM | Flash |
|----------|-----|-------|
| STM32F4 | ~8KB | ~20KB |
| ESP32 | ~20KB | ~40KB |

Exact usage depends on config (window size, queue depth, etc.)

## Performance

Typical latency: 1-5ms on local network
Throughput: Up to 10 Mbps (depends on MCU clock)

## Further Reading

- See `../include/rudp/rudp.hpp` for full API
- See `../README.md` for protocol details
