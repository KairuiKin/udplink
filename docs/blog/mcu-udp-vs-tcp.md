# 为什么你的MCU通信还在用TCP？嵌入式可靠UDP实战

> 为什么你的MCU通信还在用TCP？嵌入式可靠UDP实战

## 背景

在嵌入式开发中，我们经常遇到这样的场景：

- MCU 需要和服务器通信
- 网络环境不稳定（工业现场、户外设备）
- 对延迟敏感、需要低功耗
- 资源有限（RAM/Flash）

传统方案是用 TCP，但 TCP 在嵌入式场景下有很多问题：

### TCP 的问题

1. **开销大** - 三次握手、四次挥手，频繁通信时握手开销不可忽视
2. **队头阻塞** - TCP 保证有序，丢包会阻塞后续所有数据
3. **复杂** - 需要完整的协议栈，嵌入式移植麻烦
4. **功耗高** - 保活探测、定时器等增加功耗

## 解决方案：可靠 UDP

udplink 是一个专为嵌入式设计的可靠 UDP 库：

### 核心特性

| 特性 | 说明 |
|------|------|
| 固定内存 | 编译时确定，无动态分配 |
| 零依赖 | 纯 C++11，无外部库 |
| SACK | 选择确认，提升丢包效率 |
| 自适应 RTO | 根据网络状况自动调整超时 |
| 鉴权 | SipHash-2-4 防篡改 |
| 在线换钥 | 运行时动态更新密钥 |

### 内存占用

| 平台 | RAM | Flash |
|------|-----|-------|
| STM32F4 | ~8KB | ~20KB |
| ESP32 | ~20KB | ~40KB |

## 代码示例

### ESP32 使用

```cpp
#include "rudp/rudp.hpp"

// 配置
rudp::Config config = rudp::ConfigForProfile(rudp::kProfileLowPower);
config.enable_auth = true;
config.auth_key0 = 0x0123456789ABCDEF;
config.auth_key1 = 0xFEDCBA9876543210;

// 回调
rudp::Hooks hooks;
hooks.send = [](const uint8_t* data, uint32_t len, void*) -> int {
    return sendto(sock, data, len, 0, &dest_addr, sizeof(dest_addr));
};
hooks.get_tick_ms = []() { return esp_timer_get_time() / 1000; };

// 初始化
rudp::Endpoint endpoint;
endpoint.Init(config, hooks);

// 连接
endpoint.StartConnect("192.168.1.100", 8888);

// 主循环
while (true) {
    endpoint.Tick();
    vTaskDelay(pdMS_TO_TICKS(10));
}
```

## 性能对比

| 指标 | TCP | udplink |
|------|-----|---------|
| 握手开销 | 3-RTT | 1-RTT |
| 丢包恢复 | 慢 | 快(SACK) |
| 内存占用 | 高 | 低 |
| 功耗 | 高 | 低 |

## 适用场景

- 工业物联网网关
- 智能家居设备
- 农业监测传感器
- 无人机通信
- 机器人控制

## 总结

对于嵌入式场景，可靠 UDP 是比 TCP 更好的选择。udplink 专为 MCU 设计，零依赖、固定内存，是嵌入式通信的绝佳方案。

> GitHub: https://gitee.com/wanghaonan199105240070/udplink
