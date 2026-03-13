# Raspberry Pi UDP 示例

本示例展示如何在 Raspberry Pi (Linux) 上使用 udplink 库进行可靠 UDP 通信。

## 硬件平台

- Raspberry Pi 3B/4/5
- 运行 Raspberry Pi OS (Debian-based)

## 编译

```bash
# 安装依赖
sudo apt-get update
sudo apt-get install cmake g++ build-essential

# 编译 udplink
cd udplink
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 编译示例
cd ../examples/raspberry_pi
mkdir -p build && cd build
cmake ../.. -DCMAKE_BUILD_TYPE=Release
make
```

## 示例代码

```cpp
#include <rudp/rudp.hpp>
#include <rudp/manager.hpp>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include <arpa/inet.h>
#include <unistd.h>

// Raspberry Pi 传感器数据结构
struct SensorData {
    float temperature;
    float humidity;
    uint32_t timestamp;
    uint16_t sensor_id;
};

// UDP 套接字封装
class UdpSocket {
public:
    UdpSocket() : sock_(-1) {}
    
    bool Open(uint16_t port) {
        sock_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_ < 0) return false;
        
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(sock_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(sock_);
            return false;
        }
        return true;
    }
    
    bool SendTo(const void* data, size_t len, const char* ip, uint16_t port) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, ip, &addr.sin_addr);
        
        return sendto(sock_, data, len, 0, (struct sockaddr*)&addr, sizeof(addr)) >= 0;
    }
    
    ssize_t RecvFrom(void* buf, size_t len, char* ip, uint16_t* port) {
        struct sockaddr_in addr;
        socklen_t addrlen = sizeof(addr);
        ssize_t n = recvfrom(sock_, buf, len, 0, (struct sockaddr*)&addr, &addrlen);
        
        if (n > 0 && ip && port) {
            inet_ntop(AF_INET, &addr.sin_addr, ip, INET_ADDRSTRLEN);
            *port = ntohs(addr.sin_port);
        }
        return n;
    }
    
    void Close() {
        if (sock_ >= 0) close(sock_);
        sock_ = -1;
    }
    
    int GetFd() const { return sock_; }
    
private:
    int sock_;
};

// udplink 回调实现
class RudpCallbacks {
public:
    RudpCallbacks(UdpSocket* sock, const char* peer_ip, uint16_t peer_port)
        : sock_(sock), peer_ip_(peer_ip), peer_port_(peer_port) {}
    
    // 时间回调 - 获取系统时间(毫秒)
    uint64_t get_tick_ms() {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    }
    
    // 发送 UDP 包
    bool send_udp(const uint8_t* data, uint32_t len) {
        return sock_->SendTo(data, len, peer_ip_.c_str(), peer_port_);
    }
    
    // 投递可靠数据到应用层
    void on_rudp_packet(const uint8_t* data, uint32_t len) {
        std::cout << "[RUDP] 收到可靠数据: " << len << " bytes" << std::endl;
        
        // 解析传感器数据
        if (len >= sizeof(SensorData)) {
            const SensorData* sd = reinterpret_cast<const SensorData*>(data);
            std::cout << "  温度: " << sd->temperature << "°C" << std::endl;
            std::cout << "  湿度: " << sd->humidity << "%" << std::endl;
            std::cout << "  时间戳: " << sd->timestamp << std::endl;
        }
    }
    
    // 连接状态变化回调
    void on_state_change(rudp::State new_state) {
        const char* state_str[] = {"Closed", "SynSent", "SynAckSent", "Connected", 
                                    "FinWait", "Closing", "TimeWait"};
        std::cout << "[RUDP] 状态变化: " << state_str[static_cast<int>(new_state)] << std::endl;
    }
    
    // 连接建立回调
    void on_connected() {
        std::cout << "[RUDP] 连接已建立!" << std::endl;
    }
    
    // 连接断开回调
    void on_disconnected() {
        std::cout << "[RUDP] 连接已断开" << std::endl;
    }
    
    // 统计信息
    void on_metrics(const rudp::RuntimeMetrics& m) {
        std::cout << "[METRICS] RTT: " << m.rtt_ms << "ms, 重传率: " 
                  << (m.retrans_ratio * 100) << "%, 丢包率: " 
                  << (m.loss_ratio * 100) << "%" << std::endl;
    }

private:
    UdpSocket* sock_;
    std::string peer_ip_;
    uint16_t peer_port_;
};

// 模拟读取传感器数据
SensorData ReadSensor() {
    SensorData data;
    data.temperature = 20.0f + (rand() % 100) / 10.0f;  // 20-30°C
    data.humidity = 40.0f + (rand() % 300) / 10.0f;     // 40-70%
    data.timestamp = static_cast<uint32_t>(time(nullptr));
    data.sensor_id = 1;
    return data;
}

int main(int argc, char* argv[]) {
    bool is_server = (argc > 1 && strcmp(argv[1], "server") == 0);
    const char* peer_ip = "192.168.1.100";
    uint16_t local_port = 8888;
    uint16_t peer_port = 8889;
    
    std::cout << "=== Raspberry Pi UDP 演示 ===" << std::endl;
    std::cout << "模式: " << (is_server ? "服务器" : "客户端") << std::endl;
    
    // 初始化 UDP 套接字
    UdpSocket sock;
    if (!sock.Open(local_port)) {
        std::cerr << "Failed to open UDP socket on port " << local_port << std::endl;
        return 1;
    }
    
    // 配置 udplink
    rudp::Config config = rudp::ConfigForProfile(rudp::kBalanced);
    config.local_session_id = is_server ? 1 : 2;
    
    // 创建回调实例
    RudpCallbacks callbacks(&sock, peer_ip, peer_port);
    
    // 初始化 RUDP 端点
    rudp::Endpoint endpoint;
    endpoint.Init(config, {
        .get_tick_ms = [&callbacks]() { return callbacks.get_tick_ms(); },
        .send_udp = [&callbacks](const uint8_t* d, uint32_t l) { return callbacks.send_udp(d, l); },
        .on_rudp_packet = [&callbacks](const uint8_t* d, uint32_t l) { callbacks.on_rudp_packet(d, l); },
        .on_state_change = [&callbacks](rudp::State s) { callbacks.on_state_change(s); },
        .on_connected = [&callbacks]() { callbacks.on_connected(); },
        .on_disconnected = [&callbacks]() { callbacks.on_disconnected(); },
    });
    
    // 服务器模式: 等待连接
    if (is_server) {
        std::cout << "等待客户端连接..." << std::endl;
        // 在服务器模式下，需要先接收客户端的 SYN
    } else {
        // 客户端模式: 主动连接
        endpoint.SetPeerInfo(peer_ip, peer_port);
        endpoint.StartConnect();
    }
    
    // 主循环
    char rx_buf[4096];
    char ip[INET_ADDRSTRLEN];
    uint16_t port;
    
    while (true) {
        // 使用 select 等待数据
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock.GetFd(), &fds);
        
        struct timeval tv = {0, 100000};  // 100ms 超时
        int ret = select(sock.GetFd() + 1, &fds, nullptr, nullptr, &tv);
        
        if (ret > 0 && FD_ISSET(sock.GetFd(), &fds)) {
            ssize_t n = sock.RecvFrom(rx_buf, sizeof(rx_buf), ip, &port);
            if (n > 0) {
                // 收到 UDP 包，交给 udplink 处理
                endpoint.OnUdpPacket(reinterpret_cast<const uint8_t*>(rx_buf), n);
            }
        }
        
        // 定期调用 Tick 处理超时和重传
        endpoint.Tick();
        
        // 如果已连接，发送传感器数据
        if (endpoint.IsConnected() && !is_server) {
            static auto last_send = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_send).count() >= 5) {
                SensorData data = ReadSensor();
                endpoint.Send(reinterpret_cast<const uint8_t*>(&data), sizeof(data));
                std::cout << "已发送传感器数据" << std::endl;
                last_send = now;
            }
        }
        
        // 定期打印统计信息
        static auto last_metrics = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_metrics).count() >= 10) {
            auto metrics = endpoint.GetRuntimeMetrics();
            callbacks.on_metrics(metrics);
            last_metrics = now;
        }
    }
    
    return 0;
}
