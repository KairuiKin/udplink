# Raspberry Pi (树莓派) 示例

本目录包含在 Raspberry Pi 开发板上运行 udplink 的完整示例。

## 硬件要求

- Raspberry Pi 3B / 3B+ / 4B / 5
- SD 卡 (至少 8GB)
- 电源适配器

## 软件要求

- Raspberry Pi OS (Lite 或 Desktop)
- CMake >= 3.15
- GCC/G++ >= 7.0

## 快速开始

### 1. 在 Raspberry Pi 上安装编译工具

```bash
sudo apt-get update
sudo apt-get install cmake g++ build-essential
```

### 2. 克隆项目

```bash
git clone https://gitee.com/wanghaonan199105240070/udplink.git
cd udplink
```

### 3. 编译 udplink 库

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### 4. 编译示例

```bash
cd ../examples/raspberry_pi
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

### 5. 运行示例

服务器模式 (等待连接):
```bash
./raspberry_pi_udp server
```

客户端模式 (主动连接):
```bash
./raspberry_pi_udp client
```

## 实际应用场景

### 1. 传感器数据采集

Raspberry Pi 作为边缘网关，收集各传感器数据并通过 udplink 上传:

```
┌─────────────┐         ┌─────────────┐
│  传感器节点  │         │ Raspberry  │
│ (STM32/ESP) │────────>│   Pi 网关   │────────> 云端服务器
└─────────────┘  UDP   └─────────────┘      udplink
```

### 2. 视频流传输

结合 Raspberry Pi Camera，传输视频流:
- 低延迟可靠传输
- 可配置重传策略保证数据完整性

### 3. 工业控制

在工业环境中，可靠的 UDP 传输比 TCP 更适合:
- 更低的协议开销
- 更快的错误恢复
- 适合实时控制场景

## 性能参考

| 场景 | 延迟 | 吞吐量 |
|------|------|--------|
| 本地网络 | 1-5ms | 50+ MB/s |
| WiFi | 5-20ms | 10+ MB/s |
| 互联网 | 20-100ms | 取决于网络 |

## 注意事项

1. **防火墙**: 确保 UDP 端口 (默认 8888/8889) 已开放
2. **网络**: 建议使用有线网络以获得最佳性能
3. **电源**: 使用官方电源适配器避免性能波动
4. **散热**: 长时间运行建议加装散热片

## 扩展

- 结合 SQLite 实现数据本地缓存
- 使用 MQTT 桥接其他平台
- 添加 TLS/DTLS 加密层
