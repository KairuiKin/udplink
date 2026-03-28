# Raspberry Pi 示例

本目录包含一个基于 POSIX UDP 套接字的 `udplink` 独立示例。

## 说明

- 示例文件: `raspberry_pi_udp.cpp`
- 接口版本: 当前 `rudp::Hooks` / `Endpoint::StartConnect()` API
- 运行方式: 单独启动 `server` 与 `client`
- 如果你手里还有旧示例，可先对照 `docs/api-mapping.md` 做接口迁移。

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
