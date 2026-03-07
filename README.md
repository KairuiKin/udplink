# udplink

面向嵌入式/单片机场景的可靠 UDP 传输库（C++11）。

## 目标

- 在不引入 TCP 复杂度的前提下，提供可靠、有序传输。
- 单片机友好：固定内存占用、无动态分配、无线程依赖。
- 可商用扩展：可插拔网络适配层、可配置窗口/重传策略、统计信息可观测。

## 设计要点

- 协议内核与网络 I/O 解耦：核心依赖回调（时间、发送、投递），并支持可选临界区回调。
- 可选并发保护：支持 `enter_critical/leave_critical` 回调（适配中断或多线程临界区）。
- 选择确认（SACK 位图）：ACK + 32bit bitmap，提升丢包/乱序链路效率。
- 自适应 RTO：基于 RTT/方差估算超时，降低无效重传。
- 固定容量队列：发送/接收均为静态数组，避免堆分配和碎片化。
- 连接管理：`SYN/SYN-ACK` 建连、心跳保活、空闲超时检测。
- 轻量发送整形：`pacing_bytes_per_tick` 约束突发流量，减小 MCU 峰值负载。
- 会话防串线：每个连接携带 `session_id`，过滤过期/串线 UDP 包。
- 防重放窗口：基于 `nonce` 的 64 位滑窗，拦截重放包。
- 可选鉴权（SipHash-2-4）：`enable_auth + auth_key0/auth_key1`（或兼容 `auth_psk`），校验头部和载荷完整性。
- 在线换钥：线协议携带 `key_id`，支持双密钥并行校验与无损切换。
- 快速重传：重复 ACK 触发提前重传，降低尾延迟。
- 零拷贝发送接口：`SendZeroCopy` + `send_raw_vec`，减少 payload 拷贝。

## 目录

- `include/rudp/rudp.hpp`：公开 API
- `include/rudp/manager.hpp`：多连接管理层 API
- `src/rudp.cpp`：协议核心实现
- `src/manager.cpp`：多连接路由与会话管理
- `tests/self_test.cpp`：丢包/乱序仿真自测

## 编译

```bash
cmake -S . -B build
cmake --build build --config Release
```

## 自测

```bash
build/rudp_self_test
build/rudp_bench
build/rudp_manager_test
```

Windows 下可执行文件名为 `rudp_self_test.exe`。

## API 速览

1. `rudp::Endpoint::Init(config, hooks)` 初始化端点。
2. `rudp::Endpoint::StartConnect()` 主动发起连接。
3. `rudp::Endpoint::Send(data, len)` 在 `IsConnected()==true` 后发送可靠消息。
4. `rudp::Endpoint::SendZeroCopy(data, len)` 零拷贝发送（需实现 `send_raw_vec`）。
5. 底层收到 UDP 包后调用 `OnUdpPacket(data, len)`。
6. 周期调用 `Tick()` 触发重传、心跳与 ACK 刷新。
7. 手动切换：`SetAuthKey(new_id, k0, k1, false)` 后 `RotateTxKey(new_id)`。
8. 自动切换：`ScheduleTxKeyRotation(new_id, lead_packets)` 发送带生效点的控制帧。
9. 协议会自动回 `KEY_UPDATE_ACK`，发送端收到确认后淘汰旧 key。
10. 若 `KEY_UPDATE_ACK` 超时，发送端会延期激活点并重发；超过重试上限则取消本次换钥。

## 当前版本限制

- 以“消息”为单位，不做流式字节拼接。
- 已实现完整性鉴权（SipHash-2-4），未提供机密性加密（商用建议外层叠加 DTLS/WireGuard/IPsec）。
- 单连接协议内核；多连接可在业务层按会话维护多个 `Endpoint` 实例。
