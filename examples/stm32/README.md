# STM32 示例说明

本目录提供基于 STM32F4 + lwIP/W5500 的 `udplink` 参考模板。

## 目标环境

- STM32 HAL 或等价 BSP
- UDP 发送与轮询接收能力
- 毫秒级系统时钟

## 你需要替换的部分

- `HardwareInit()`
- `PlatformNowMs()`
- `PlatformSendUdp()`
- `PlatformPollUdp()`

## 适配清单

1. 确认系统时钟或 `HAL_GetTick()` 可提供毫秒时间。
2. 确认 UDP 发送路径能完整发出 `send_raw` 数据。
3. 确认接收轮询路径不会漏掉入站 UDP 包。
4. 确认 `Tick()` 以稳定节奏被调用。
5. 确认主动建连侧会调用 `StartConnect()`。
6. 在首次 bring-up 阶段只测试单个 peer 和小 payload。

## 本地验收建议

- 先在不改 profile 的前提下跑通连接。
- 再验证一条应用层 payload 是否能到达 `OnDeliver`。
- 最后才开始调窗口、重传或认证参数。

## CI 范围

- 主仓库 CI 现在会对该模板做 host-side compile-only 检查。
- 这只能覆盖 API/模板语法一致性，不能替代 HAL、lwIP、W5500 等真实工程验证。

## 相关文件

- 模板源码: `examples/stm32/stm32_f4_udp.cpp`
- API 对照: `docs/api-mapping.md`
