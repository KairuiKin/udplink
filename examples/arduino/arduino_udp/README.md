# Arduino 示例

本目录包含在 Arduino 开发板上运行 udplink 的完整示例。

## 硬件支持

| 开发板 | RAM | Flash | 以太网 | 状态 |
|--------|-----|-------|--------|------|
| Arduino UNO R3 | 2KB | 32KB | W5100 | ✅ 已测试 |
| Arduino Mega 2560 | 8KB | 256KB | W5100/W5500 | ✅ 已测试 |
| Arduino Nano | 2KB | 32KB | ENC28J60 | ✅ 已测试 |
| Arduino MKR系列 | 32KB | 256KB | 内置WiFi | 🔄 待测试 |

## 软件要求

- Arduino IDE 1.8+ 或 PlatformIO
- Ethernet2 库 或 Ethernet 库 (内置)
- rudp 库

## 安装

### 方式 1: Arduino IDE

1. 复制 `arduino_udp` 文件夹到你的 Arduino 项目目录
2. 安装依赖库:
   - `Ethernet` (内置)
   - `SPI` (内置)

### 方式 2: PlatformIO

创建 `platformio.ini`:
```ini
[env:uno]
platform = atmelavr
board = uno
framework = arduino
lib_deps =
    Ethernet
build_flags =
    -DARDUINO=10800
```

## 硬件接线

### Ethernet W5100/W5500

```
Arduino -> W5100/W5500
-----------------------
3.3V   -> VCC
GND    -> GND
Pin 10 -> CS
Pin 11 -> MOSI
Pin 12 -> MISO
Pin 13 -> SCK
```

### 传感器

```
传感器 -> Arduino
-----------------
VCC    -> 5V
GND    -> GND
DOUT   -> A0 (温度)
DOUT   -> A1 (湿度)
```

## 配置

在代码中修改以下配置:

```cpp
// IP 地址 (根据你的网络修改)
IPAddress localIP(192, 168, 1, 177);
IPAddress serverIP(192, 168, 1, 100);

// 端口
const uint16_t LOCAL_PORT = 8888;
const uint16_t SERVER_PORT = 8889;
```

## 编译与上传

### Arduino IDE

1. 打开 `arduino_udp.ino`
2. 选择开发板: 工具 -> 开发板 -> Arduino Uno
3. 选择端口: 工具 -> 端口 -> COMx
4. 上传 (Ctrl+U)

### PlatformIO

```bash
pio run --target upload
```

## 串口输出

使用 Arduino IDE 串口监视器 (115200 baud):

```
=== Arduino RUDP 示例 ===
编译时间: 12:00:00
初始化以太网...
本地IP: 192.168.1.177
网络初始化完成
发起连接...
[RUDP] 状态: SynSent
[RUDP] 状态: Connected!
[APP] 发送传感器数据: T=25.3C, H=65.2%
[STATS] 发送: 5, 接收: 3, 连接: 是
```

## 协议说明

### 数据包格式

| 字节 | 字段 | 说明 |
|------|------|------|
| 0 | type | 0x01=查询, 0x02=传感器数据 |
| 1-2 | temperature | 温度 (int16, 放大10倍) |
| 3-4 | humidity | 湿度 (uint16, 放大10倍) |
| 5-8 | timestamp | 时间戳 (ms) |

### 命令

- `0x01000000`: 查询传感器数据
- 响应: 发送当前传感器数据

## 性能优化

Arduino 资源有限，以下是关键优化:

1. **静态内存**: 所有缓冲区使用全局/静态数组
2. **固定大小**: 避免任何动态内存分配
3. **精简配置**: 使用 `kLowPower` 配置档位
4. **最小Tick周期**: 10ms Tick 间隔

### 内存使用

```
RAM:
  - UDP 接收缓冲区: 512 bytes
  - RUDP 发送缓冲区: 256 bytes
  - RUDP 接收缓冲区: 256 bytes
  - RUDP 内部状态: ~200 bytes
  - Ethernet 缓冲区: ~400 bytes
  =-----------------
  总计: ~1624 bytes (UNO: 81%)

Flash:
  - 代码: ~20KB
  - Ethernet 库: ~8KB
  =-----------------
  总计: ~28KB (UNO: 87%)
```

## 故障排除

### 连接失败

1. 检查网线是否插好
2. 确认 IP 地址不冲突
3. 检查防火墙是否阻止 UDP

### 串口无输出

1. 检查波特率是否为 115200
2. 按一下 Arduino 复位按钮

### 内存不足

1. 减小缓冲区大小
2. 关闭串口调试输出
3. 使用 Mega 2560

## 扩展应用

- 温室监控系统
- 智能灌溉控制
- 环境数据采集
- 工业传感器网关
