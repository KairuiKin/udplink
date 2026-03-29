# Raspberry Pi 示例

本目录包含一个基于 POSIX UDP 套接字的 `udplink` 独立示例。

## 说明

- 示例文件: `raspberry_pi_udp.cpp`
- 接口版本: 当前 `rudp::Hooks` / `Endpoint::StartConnect()` API
- 运行方式: 单独启动 `server` 与 `client`
- 如果你手里还有旧示例，可先对照 `docs/api-mapping.md` 做接口迁移。

## 适配清单

1. 确认本机防火墙未拦截示例使用的 UDP 端口。
2. 如果跨机器运行，先确认 `peer_ip` 能互通。
3. 确认主动侧使用 `client` 模式并调用 `StartConnect()`。
4. 确认接收循环把每个 UDP 数据报都交给 `OnUdpPacket(...)`。
5. 观察日志中状态是否最终进入 `ConnectionState::kConnected`。
6. 首次验证时先发送单个小结构体 payload，再扩展业务数据。

## 构建

```bash
cd examples/raspberry_pi
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## 运行

终端 1:

```bash
./build/raspberry_pi_udp server 127.0.0.1
```

终端 2:

```bash
./build/raspberry_pi_udp client 127.0.0.1
```

如果跨机器运行，把 `127.0.0.1` 替换为对端 Raspberry Pi 或 Linux 主机的 IP。

## 本地验收建议

- 先用回环地址跑通 `server/client`。
- 再切换到两台机器验证实际网络环境。
- 如果需要长期运行，优先补充进程管理和日志收集，而不是直接修改协议参数。
