# ESP32 示例说明

本目录提供基于 ESP-IDF + lwIP sockets 的 `udplink` 参考模板。

## 目标环境

- ESP-IDF 工程
- UDP socket 可用
- 可提供毫秒级计时源

## 你需要替换的部分

- 网络初始化与联网流程
- `kPeerIp` / `kLocalPort` / `kPeerPort`
- `ReceiveTask` 的任务栈与优先级
- 如有需要，`SendRawVec` 的缓冲策略

## 适配清单

1. 确认 `socket()` / `bind()` / `recvfrom()` 可正常工作。
2. 把 `kPeerIp` 改成实际对端地址。
3. 检查 `NowMs()` 是否使用稳定的毫秒时钟。
4. 确认接收任务会把每个 UDP 包都交给 `OnUdpPacket(...)`。
5. 确认主循环或任务中持续调用 `g_endpoint.Tick()`。
6. 观察日志中状态是否最终进入 `ConnectionState::kConnected`。

## 本地验收建议

- 先对接桌面 `socket_posix` 或 `socket_winsock` 示例验证链路。
- 首次只发送小 payload，避免把网络和内存问题混在一起。
- 如果链路不稳，先保留 `ConfigProfile::kLowPower` 默认值，不要同时修改多个传输参数。

## CI 范围

- 主仓库 CI 现在会做 host-side compile-only 检查。
- 这能防止模板与当前 API 漂移，但不能代替 ESP-IDF 真机或 SDK 编译验证。

## 相关文件

- 模板源码: `examples/esp32/esp32_udp.cpp`
- API 对照: `docs/api-mapping.md`
