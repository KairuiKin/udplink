# Arduino 示例

本目录提供基于当前 `udplink` API 的 Arduino 参考模板。

## 状态说明

- 该示例用于演示 `rudp::Hooks`、`Endpoint::StartConnect()`、`SendStatus` 的接法。
- 它不在主仓库 CI 中编译。
- 板卡、以太网芯片、库版本不同，通常还需要你本地做少量适配。

## 建议目标板

| 开发板 | 说明 |
|--------|------|
| Arduino Mega 2560 | 更适合调试，RAM 余量更大 |
| Arduino UNO R3 | 仅适合精简配置 |
| MKR 系列 | 需要按实际网络库调整 |

## 软件要求

- Arduino IDE 1.8+ 或 PlatformIO（当前仅建议作为本地 bring-up 选项）
- `Ethernet` 与 `SPI` 库
- `udplink` 头文件与源码

## 配置项

在 `arduino_udp.ino` 中调整以下内容：

```cpp
IPAddress kLocalIp(192, 168, 1, 177);
IPAddress kPeerIp(192, 168, 1, 100);
const uint16_t kLocalPort = 8888;
const uint16_t kPeerPort = 8889;
```

## 适配清单

1. 确认目标板的 `Ethernet` 库与以太网芯片匹配。
2. 确认 `kLocalIp`、`kPeerIp`、端口配置符合你的局域网环境。
3. 确认 `g_udp.begin(kLocalPort)` 成功。
4. 确认 `millis()` 精度足够支撑 `Tick()` 的 10ms 节奏。
5. 串口观察状态值是否最终进入 `2`（`kConnected`）。
6. 先验证一个小 payload，再考虑增大 `max_payload` 或自定义 profile。

## API 重点

- 发送结果需要判断 `rudp::SendStatus::kOk`。
- 主动建连使用 `g_endpoint.StartConnect()`。
- 周期性调用 `g_endpoint.Tick()`。
- 应用层收包通过 `OnDeliver` 回调进入。
- 如果要从旧示例迁移，优先参考 `docs/api-mapping.md`。

## PlatformIO 定位

- 当前项目将 PlatformIO 视为 Arduino 示例的本地接入路径，而不是主仓库 CI 保证项。
- 仓库当前提供一个 `Arduino Mega 2560 + Ethernet` 的最小本地工程骨架。
- 适合做单板 bring-up、串口观察和网络参数调试。
- 如果你需要正式维护的 PlatformIO 工程矩阵，建议先在 issue 中说明目标板、网卡和库版本。

## PlatformIO 快速开始

```bash
cd examples/arduino/arduino_udp
pio run -e megaatmega2560_w5100
pio device monitor -b 115200
```

目录说明：

- `platformio.ini`：最小本地工程配置，默认环境为 `megaatmega2560_w5100`，并为 AVR 路径收紧 `RUDP_MAX_FRAME=128` / `RUDP_MAX_QUEUE=8`
- `src/main.cpp`：直接复用 `arduino_udp.ino`
- `src/rudp_repo.cpp` 与 `src/manager_repo.cpp`：把仓库根目录的 `src/rudp.cpp`、`src/manager.cpp` 编进当前工程

注意：

- 这个骨架的目标是减少本地 bring-up 摩擦，不是提供独立发布的 Arduino 库封装。
- 如果你改了 `arduino_udp.ino`，PlatformIO 路径会自动复用同一份示例代码。
- 如果本地网卡或库版本不同，优先先改 `platformio.ini` 和网络参数，再看应用层逻辑。

## 调试建议

- 先在串口观察连接状态是否从 `0/1` 变化到 `2`。
- 如果 RAM 紧张，优先减小 `max_payload` 与应用缓冲区。
- 如果链路不稳，可改用 `ConfigProfile::kLossyLink` 做对比验证。
- 如果 `beginPacket` / `write` / `endPacket` 行为异常，先排除库版本或网卡兼容性问题。

## 参考 bring-up 路径

- 推荐首先按 `docs/arduino-mega-w5100-bringup.md` 跑通 Arduino Mega 2560 + W5100 + PlatformIO + host peer 这一条最小闭环路径。
- 该文档给出了主机侧 `rudp_example_udp_peer` 的启动命令和预期串口/控制台输出。
