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

- Arduino IDE 1.8+ 或 PlatformIO
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

## API 重点

- 发送结果需要判断 `rudp::SendStatus::kOk`。
- 主动建连使用 `g_endpoint.StartConnect()`。
- 周期性调用 `g_endpoint.Tick()`。
- 应用层收包通过 `OnDeliver` 回调进入。
- 如果要从旧示例迁移，优先参考 `docs/api-mapping.md`。

## 调试建议

- 先在串口观察连接状态是否从 `0/1` 变化到 `2`。
- 如果 RAM 紧张，优先减小 `max_payload` 与应用缓冲区。
- 如果链路不稳，可改用 `ConfigProfile::kLossyLink` 做对比验证。
