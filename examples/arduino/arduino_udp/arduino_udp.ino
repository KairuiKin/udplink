/**
 * @file arduino_udp.ino
 * @brief Arduino UNO/Mega/Nano 上的 udplink 可靠 UDP 示例
 * 
 * 硬件要求:
 * - Arduino UNO R3 / Mega 2560 / Nano
 * - Ethernet Shield (W5100/W5200) 或 WiFi Shield (ESP8266)
 * 
 * 本示例展示如何在资源受限的 Arduino 平台上使用 udplink
 * 关键优化:
 * - 静态内存分配，无动态 new/delete
 * - 固定缓冲区大小
 * - 最小化堆栈使用
 */

#include <rudp/rudp.hpp>
#include <SPI.h>
#include <Ethernet.h>

// ==================== 配置 ====================

// 网络配置 (根据你的网络修改)
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress localIP(192, 168, 1, 177);  // Arduino IP
IPAddress serverIP(192, 168, 1, 100); // 服务器 IP

// UDP 端口
const uint16_t LOCAL_PORT = 8888;
const uint16_t SERVER_PORT = 8889;

// 传感器引脚
const int TEMP_PIN = A0;
const int HUMIDITY_PIN = A1;

// ==================== 静态缓冲区 ====================
// 关键: 使用静态/全局数组，避免堆分配

// UDP 接收缓冲区 (根据MCU资源调整)
static uint8_t udp_rx_buf[512];

// RUDP 发送缓冲区
static uint8_t rudp_tx_buf[256];

// RUDP 接收缓冲区
static uint8_t rudp_rx_buf[256];

// ==================== RUDP 实例 ====================
static rudp::Endpoint rudp_endpoint;

// 统计计数
static uint32_t g_send_count = 0;
static uint32_t g_recv_count = 0;
static uint32_t g_last_rudp_tick = 0;

// ==================== 回调函数 ====================

/**
 * 获取系统 tick (毫秒)
 * Arduino 上使用 millis()
 */
static uint32_t get_tick_ms() {
    return millis();
}

/**
 * 发送 UDP 包
 * 使用 EthernetUDP 类发送
 */
static EthernetUDP* g_udp = nullptr;

static bool send_udp(const uint8_t* data, uint32_t len) {
    if (!g_udp) return false;
    
    g_udp->beginPacket(serverIP, SERVER_PORT);
    size_t written = g_udp->write(data, len);
    g_udp->endPacket();
    
    return written == len;
}

/**
 * 收到可靠数据包
 */
static void on_rudp_packet(const uint8_t* data, uint32_t len) {
    g_recv_count++;
    
    // 简单打印收到的数据 (串口调试)
    Serial.print("[RUDP] 收到数据: ");
    Serial.print(len);
    Serial.println(" bytes");
    
    // 解析命令
    if (len >= 4) {
        uint32_t cmd = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
        
        if (cmd == 0x01000000) {  // 查询传感器
            send_sensor_data();
        }
    }
}

/**
 * 状态变化回调
 */
static void on_state_change(rudp::State state) {
    Serial.print("[RUDP] 状态: ");
    switch (state) {
        case rudp::State::kClosed: Serial.println("Closed"); break;
        case rudp::State::kSynSent: Serial.println("SynSent"); break;
        case rudp::State::kSynAckSent: Serial.println("SynAckSent"); break;
        case rudp::State::kConnected: Serial.println("Connected!"); break;
        case rudp::State::kFinWait: Serial.println("FinWait"); break;
        case rudp::State::kClosing: Serial.println("Closing"); break;
        case rudp::State::kTimeWait: Serial.println("TimeWait"); break;
    }
}

/**
 * 连接建立回调
 */
static void on_connected() {
    Serial.println("[RUDP] 连接已建立!");
}

/**
 * 连接断开回调
 */
static void on_disconnected() {
    Serial.println("[RUDP] 连接已断开");
}

// ==================== 应用函数 ====================

/**
 * 读取传感器数据并发送
 */
static void send_sensor_data() {
    // 读取模拟传感器
    int temp_raw = analogRead(TEMP_PIN);
    int humi_raw = analogRead(HUMIDITY_PIN);
    
    // 转换为实际值 (根据传感器型号调整)
    float temperature = map(temp_raw, 0, 1023, 0, 500) / 10.0;  // 0-50°C
    float humidity = map(humi_raw, 0, 1023, 0, 1000) / 10.0;      // 0-100%
    
    // 构建数据包
    uint8_t payload[16];
    payload[0] = 0x02;  // 传感器数据类型
    
    // 温度 (int16, 放大10倍)
    int16_t temp_int = (int16_t)(temperature * 10);
    payload[1] = temp_int & 0xFF;
    payload[2] = (temp_int >> 8) & 0xFF;
    
    // 湿度 (uint16, 放大10倍)
    uint16_t humi_int = (uint16_t)(humidity * 10);
    payload[3] = humi_int & 0xFF;
    payload[4] = (humi_int >> 8) & 0xFF;
    
    // 时间戳
    uint32_t ts = millis();
    payload[5] = ts & 0xFF;
    payload[6] = (ts >> 8) & 0xFF;
    payload[7] = (ts >> 16) & 0xFF;
    payload[8] = (ts >> 24) & 0xFF;
    
    // 发送
    if (rudp_endpoint.Send(payload, 16)) {
        g_send_count++;
        Serial.print("[APP] 发送传感器数据: T=");
        Serial.print(temperature, 1);
        Serial.print("C, H=");
        Serial.print(humidity, 1);
        Serial.println("%");
    }
}

/**
 * 初始化 Ethernet 和 RUDP
 */
static bool init_networking() {
    Serial.println("初始化以太网...");
    
    // 尝试 DHCP
    if (Ethernet.begin(mac) == 0) {
        Serial.println("DHCP 失败，使用静态IP");
        Ethernet.begin(mac, localIP);
    }
    
    // 等待物理连接
    delay(1000);
    
    Serial.print("本地IP: ");
    Serial.println(Ethernet.localIP());
    
    // 初始化 UDP
    static EthernetUDP udp;
    if (!udp.begin(LOCAL_PORT)) {
        Serial.println("UDP 初始化失败!");
        return false;
    }
    g_udp = &udp;
    
    // 配置 RUDP
    rudp::Config config = rudp::ConfigForProfile(rudp::kLowPower);
    config.local_session_id = 1;
    config.peer_session_id = 2;
    
    // 设置缓冲区
    config.tx_buffer_size = 256;
    config.rx_buffer_size = 256;
    config.max_packet_size = 256;
    
    // 初始化 RUDP
    rudp_endpoint.Init(config, {
        .get_tick_ms = get_tick_ms,
        .send_udp = send_udp,
        .on_rudp_packet = on_rudp_packet,
        .on_state_change = on_state_change,
        .on_connected = on_connected,
        .on_disconnected = on_disconnected,
    });
    
    // 设置服务器信息
    rudp_endpoint.SetPeerInfo(serverIP, SERVER_PORT);
    
    Serial.println("网络初始化完成");
    return true;
}

// ==================== Arduino 标准函数 ====================

void setup() {
    // 串口初始化
    Serial.begin(115200);
    while (!Serial && millis() < 3000); // 等待串口，最多3秒
    
    Serial.println("\n=== Arduino RUDP 示例 ===");
    Serial.print("编译时间: ");
    Serial.println(__TIME__);
    
    // 初始化网络和 RUDP
    if (!init_networking()) {
        Serial.println("初始化失败!");
        while (true) {
            delay(1000); // 停止
        }
    }
    
    // 主动连接
    Serial.println("发起连接...");
    rudp_endpoint.StartConnect();
}

void loop() {
    // 检查 Ethernet 状态
    switch (Ethernet.maintain()) {
        case 1: Serial.println("renew 失败"); break;
        case 2: Serial.println("renew 成功"); break;
        case 3: Serial.println("绑定失败"); break;
        case 4: Serial.println("绑定成功"); break;
    }
    
    // 接收 UDP 数据
    int packetSize = g_udp->parsePacket();
    if (packetSize > 0) {
        int len = g_udp->read(udp_rx_buf, sizeof(udp_rx_buf));
        if (len > 0) {
            // 交给 RUDP 处理
            rudp_endpoint.OnUdpPacket(udp_rx_buf, len);
        }
    }
    
    // RUDP Tick 处理
    uint32_t now = millis();
    if (now - g_last_rudp_tick >= 10) {  // 10ms 周期
        rudp_endpoint.Tick();
        g_last_rudp_tick = now;
        
        // 连接后每 30 秒发送一次数据
        if (rudp_endpoint.IsConnected() && (now % 30000) < 20) {
            static uint32_t last_send = 0;
            if (now - last_send > 30000) {
                send_sensor_data();
                last_send = now;
            }
        }
    }
    
    // 定期打印统计
    static uint32_t last_stats = 0;
    if (now - last_stats > 10000) {
        Serial.print("[STATS] 发送: ");
        Serial.print(g_send_count);
        Serial.print(", 接收: ");
        Serial.print(g_recv_count);
        Serial.print(", 连接: ");
        Serial.println(rudp_endpoint.IsConnected() ? "是" : "否");
        last_stats = now;
    }
    
    // 小延迟避免 CPU 占用过高
    delay(1);
}
