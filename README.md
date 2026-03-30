# udplink

面向嵌入式与 MCU 场景的可靠 UDP 库，基于 C++11，强调固定内存、可预测行为和可移植的 I/O 抽象。

## 当前状态

- 核心 C++ 协议栈已可在 Windows / Linux / macOS 构建与测试。
- 最小 C ABI 已落地到 `include/rudp/rudp_c.h` / `src/rudp_c.cpp`。
- 安装导出、CMake package、pkg-config、C/C++ 下游消费路径已打通。
- Windows 路径当前最完整：`ctest`、install-consume、C API example、release check 均已验证。
- PlatformIO 目前仍是 Arduino-first 的窄路径，本仓库没有承诺广泛板卡矩阵。
- 当前 C ABI 已实现，但还没有被正式声明为长期稳定；仓库当前推荐仍然是 `wait`。

## 文档入口

- 在线落地页: https://kairuikin.github.io/udplink/
- 在线文档浏览器: https://kairuikin.github.io/udplink/guide.html
- 当前项目状态: `docs/project-status.md`
- C API 快速开始: `docs/c-api-quickstart.md`
- Arduino Mega W5100 bring-up: `docs/arduino-mega-w5100-bringup.md`
- 构建、测试、安装与 API 概览以本 README 为准。

## 设计目标

- 在 UDP 之上提供可靠、有序传输，而不引入完整 TCP 的复杂度。
- 适配 MCU：固定槽位、无热路径堆分配、无线程硬依赖。
- 保持宿主环境解耦：时间、发包、投递都通过 hooks 注入。
- 允许在不同实时性/功耗/链路质量场景下通过 profile 调参。

## 核心能力

- `rudp::Endpoint` 单连接协议端点
- `rudp::ConnectionManager` 多连接路由与生命周期管理
- 选择性 ACK：累计 ACK + 32-bit bitmap
- RTT / RTO 自适应重传
- 心跳、空闲超时、快速重传
- 可选 pacing
- `session_id` 隔离
- `nonce` 抗重放窗口
- SipHash-2-4 鉴权与在线密钥轮换
- `SendZeroCopy` + `send_raw_vec` 零拷贝发送路径
- 最小 C ABI 包装层，供 C / FFI / 后续 Rust 方向使用

## 仓库布局

- `include/rudp/rudp.hpp`: C++ 公共端点 API
- `include/rudp/manager.hpp`: 多连接管理器 API
- `include/rudp/rudp_c.h`: 最小 C ABI 头文件
- `src/rudp.cpp`: 协议实现
- `src/manager.cpp`: 连接管理器实现
- `src/rudp_c.cpp`: C ABI wrapper 实现
- `tests/`: 自测、回归、bench、install-consume
- `examples/`: C++ / C / 嵌入式参考示例
- `docs/`: ABI 决策、发布、板级 bring-up、项目状态等文档

## 构建

### 通用

```bash
cmake -S . -B build
cmake --build build --config Release
```

## 测试

### Unix-like

```bash
ctest --test-dir build --output-on-failure
```

### Windows PowerShell

```powershell
ctest --test-dir build -C Release --output-on-failure
```

当前注册到 `ctest` 的核心测试包括：

- `rudp_self_test`
- `rudp_reliability_test`
- `rudp_manager_test`
- `rudp_c_api_test`
- `rudp_bench`

## 安装与下游消费路径

### 安装导出

```bash
cmake -S . -B build-install -DRUDP_INSTALL=ON -DRUDP_BUILD_TESTS=OFF -DRUDP_BUILD_EXAMPLES=OFF
cmake --build build-install --config Release
cmake --install build-install --config Release --prefix ./stage
```

安装产物包括：

- `rudpConfig.cmake` / `rudpTargets.cmake`
- `rudp.pc`
- 公共头文件，包括 `include/rudp/rudp_c.h`

### Unix-like 下游消费

```bash
cmake -S tests/install_consume -B build-consume -DCMAKE_PREFIX_PATH=./stage
cmake -S tests/install_consume_c -B build-consume-c -DCMAKE_PREFIX_PATH=./stage
cmake -S examples/c_api/install_consume -B build-example-c-api -DCMAKE_PREFIX_PATH=./stage
cmake --build build-consume --config Release
cmake --build build-consume-c --config Release
cmake --build build-example-c-api --config Release
```

### Windows PowerShell 下游消费

```powershell
cmake -S tests/install_consume -B build-consume -Drudp_DIR:PATH="$PWD\stage\lib\cmake\rudp"
cmake -S tests/install_consume_c -B build-consume-c -Drudp_DIR:PATH="$PWD\stage\lib\cmake\rudp"
cmake -S examples/c_api/install_consume -B build-example-c-api -Drudp_DIR:PATH="$PWD\stage\lib\cmake\rudp"
cmake --build build-consume --config Release
cmake --build build-consume-c --config Release
cmake --build build-example-c-api --config Release
```

## Windows 完成定义

当前在 Windows 上，以下路径已经实际验证通过：

- `ctest --test-dir build -C Release --output-on-failure`
- `tests/install_consume`
- `tests/install_consume_c`
- `examples/c_api/install_consume`
- `python scripts/release_check.py` 的关键验证链路

如果你只关心 Windows 主线，建议把下面这三项视为最小完成定义：

1. 主项目 `Release` 构建成功。
2. `ctest --test-dir build -C Release --output-on-failure` 全绿。
3. install-consume 三条路径都能配置、构建并运行。

## 示例

桌面示例：

- `examples/basic_endpoint.cpp`
- `examples/basic_manager.cpp`
- `examples/socket_posix.cpp`
- `examples/socket_winsock.cpp`
- `examples/socket_peer.cpp`

嵌入式参考模板：

- `examples/stm32/stm32_f4_udp.cpp`
- `examples/esp32/esp32_udp.cpp`
- `examples/raspberry_pi/raspberry_pi_udp.cpp`
- `examples/arduino/arduino_udp/arduino_udp.ino`

C API 安装消费示例：

- `examples/c_api/install_consume/main.c`

## C ABI 现状

当前仓库在 C ABI 上的关键立场是：

- C ABI 已实现并已进入测试、安装消费和维护示例路径。
- 但仓库当前还没有正式宣布它是长期稳定 ABI。
- `examples/c_api/install_consume/` 目前是强证据，但还没有被维护者正式提升为“首个受支持的 downstream baseline”。

相关文档：

- `docs/c-api-quickstart.md`
- `docs/c-abi-compatibility.md`
- `docs/c-abi-stability-gate.md`
- `docs/c-abi-stability-assessment.md`
- `docs/c-abi-downstream-baseline-decision.md`
- `docs/c-abi-stability-decision-playbook.md`

## API 快速概览

1. `rudp::Endpoint::Init(config, hooks)` 用于初始化端点。
2. `rudp::Endpoint::StartConnect()` 用于主动发起握手。
3. `rudp::Endpoint::Send(data, len)` 在连接建立后发送可靠消息。
4. `rudp::Endpoint::SendZeroCopy(data, len)` 在 `send_raw_vec` 可用时避免额外 payload 拷贝。
5. 每个入站 UDP datagram 都应交给 `OnUdpPacket(data, len)`。
6. 周期性调用 `Tick()` 以驱动重传、心跳、pacing 和 ACK flush。
7. 使用 `Endpoint::GetRuntimeMetrics()` 查看队列、RTO、RTT、重传和丢弃指标。

## Manager API 快速概览

1. `rudp::ConnectionManager::Init(config, hooks)` 用于初始化多连接路由。
2. `Open(key, start_connect)` 创建或查找带 key 的端点，`Remove(key)` 用于关闭它。
3. `OnUdpPacket(key, data, len)` 按 session key 路由入站 datagram。
4. 新代码应优先使用 manager 级别的 send/query API。
5. `Open()/Find()` 返回的 `Endpoint*` 仍然是兼容性逃生口，而不是长期推荐方向。

## C API 快速概览

1. 使用 `#include <rudp/rudp_c.h>` 引入最小 C ABI 接口。
2. 用 `rudp_default_config_v1()` 或 `rudp_config_for_profile_v1()` 初始化配置。
3. 在 `rudp_endpoint_init()` 之前先调用 `rudp_endpoint_create()`。
4. 通过 `rudp_endpoint_start_connect()` 和 `rudp_endpoint_tick()` 驱动握手与定时逻辑。
5. 通过 `rudp_endpoint_on_udp_packet()` 喂入入站 UDP datagram。
6. 使用 `rudp_endpoint_send()` 发送数据。
7. 通过公开的 C getter 查询状态与运行时指标。

## PlatformIO / 嵌入式现状

- PlatformIO 当前只提供一条窄路径：Arduino Mega 2560 + W5100。
- 这条路径已经具备本地构建能力，但不等于广泛嵌入式支持承诺。
- 首条板级 bring-up 路线的执行文档在：
  - `docs/arduino-mega-w5100-bringup.md`
  - `docs/arduino-mega-w5100-maintainer-run.md`

## 一键本地发布检查

```bash
python scripts/release_check.py
```

这个脚本会串起：

- docs snippet 同步
- C ABI 文档状态检查
- 主项目构建与测试
- 安装导出
- C / C++ install-consume
- 维护中的 C API consumer example
