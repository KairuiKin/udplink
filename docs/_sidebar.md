# Getting Started

Welcome to udplink!

## Quick Install

```bash
# Clone
git clone https://github.com/KairuiKin/udplink.git
cd udplink

# Build
cmake -S . -B build
cmake --build build --config Release

# Test
./build/rudp_self_test
```

## Examples

See `examples/` directory:
- `basic_endpoint.cpp` - Simple point-to-point
- `socket_posix.cpp` - POSIX UDP
- `socket_winsock.cpp` - Windows UDP
- `stm32/stm32_f4_udp.cpp` - STM32F4
- `esp32/esp32_udp.cpp` - ESP32

## Documentation

- [API Reference](api.md)
- [Protocol Design](protocol.md)
- [Embedded Guide](embedded.md)

## Support

- Issues: https://github.com/KairuiKin/udplink/issues
