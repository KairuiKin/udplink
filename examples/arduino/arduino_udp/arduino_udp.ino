#include <Ethernet.h>
#include <SPI.h>

#include <rudp/rudp.hpp>

namespace {

byte kMac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress kLocalIp(192, 168, 1, 177);
IPAddress kPeerIp(192, 168, 1, 100);

const uint16_t kLocalPort = 8888;
const uint16_t kPeerPort = 8889;
const uint8_t kTempPin = A0;
const uint8_t kHumidityPin = A1;

EthernetUDP g_udp;
rudp::Endpoint g_endpoint;
uint32_t g_send_count = 0;
uint32_t g_recv_count = 0;
uint32_t g_last_tick_ms = 0;
rudp::ConnectionState g_last_state = rudp::ConnectionState::kDisconnected;

static uint32_t NowMs(void*) {
    return millis();
}

static bool SendRaw(void*, const uint8_t* data, uint16_t len) {
    g_udp.beginPacket(kPeerIp, kPeerPort);
    const size_t written = g_udp.write(data, len);
    g_udp.endPacket();
    return written == len;
}

static void OnDeliver(void*, const uint8_t* data, uint16_t len) {
    ++g_recv_count;
    Serial.print("[RUDP] recv ");
    Serial.print(len);
    Serial.print(" bytes: ");
    Serial.write(data, len);
    Serial.println();
}

static void LogStateChange() {
    const rudp::ConnectionState state = g_endpoint.GetConnectionState();
    if (state == g_last_state) {
        return;
    }
    Serial.print("[RUDP] state=");
    Serial.println(static_cast<int>(state));
    g_last_state = state;
}

static void SendSensorData() {
    uint8_t payload[9];
    payload[0] = 0x02;

    const int temp_raw = analogRead(kTempPin);
    const int humidity_raw = analogRead(kHumidityPin);
    const int16_t temp = static_cast<int16_t>(map(temp_raw, 0, 1023, 0, 500));
    const uint16_t humidity = static_cast<uint16_t>(map(humidity_raw, 0, 1023, 0, 1000));
    const uint32_t ts = millis();

    payload[1] = static_cast<uint8_t>(temp & 0xFF);
    payload[2] = static_cast<uint8_t>((temp >> 8) & 0xFF);
    payload[3] = static_cast<uint8_t>(humidity & 0xFF);
    payload[4] = static_cast<uint8_t>((humidity >> 8) & 0xFF);
    payload[5] = static_cast<uint8_t>(ts & 0xFF);
    payload[6] = static_cast<uint8_t>((ts >> 8) & 0xFF);
    payload[7] = static_cast<uint8_t>((ts >> 16) & 0xFF);
    payload[8] = static_cast<uint8_t>((ts >> 24) & 0xFF);

    if (g_endpoint.Send(payload, sizeof(payload)) == rudp::SendStatus::kOk) {
        ++g_send_count;
        Serial.print("[APP] sent sensor packet #");
        Serial.println(g_send_count);
    }
}

static bool InitNetworking() {
    Serial.println("Initializing Ethernet...");
    if (Ethernet.begin(kMac) == 0) {
        Ethernet.begin(kMac, kLocalIp);
    }
    delay(1000);

    Serial.print("Local IP: ");
    Serial.println(Ethernet.localIP());

    if (!g_udp.begin(kLocalPort)) {
        Serial.println("UDP bind failed");
        return false;
    }

    rudp::Config cfg = rudp::ConfigForProfile(rudp::ConfigProfile::kLowPower);
    cfg.mtu = 128;
    cfg.max_payload = 96;
    cfg.enable_auth = true;
    cfg.auth_key0 = 0x0123456789ABCDEFull;
    cfg.auth_key1 = 0xFEDCBA9876543210ull;

    rudp::Hooks hooks = {0, NowMs, SendRaw, 0, OnDeliver, 0, 0};
    if (!g_endpoint.Init(cfg, hooks)) {
        Serial.println("rudp init failed");
        return false;
    }
    return true;
}

}  // namespace

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000) {
    }

    Serial.println("\n=== Arduino udplink example ===");
    if (!InitNetworking()) {
        while (true) {
            delay(1000);
        }
    }
    g_endpoint.StartConnect();
}

void loop() {
    switch (Ethernet.maintain()) {
        case 1: Serial.println("DHCP renew failed"); break;
        case 2: Serial.println("DHCP renew ok"); break;
        case 3: Serial.println("DHCP rebind failed"); break;
        case 4: Serial.println("DHCP rebind ok"); break;
    }

    const int packet_size = g_udp.parsePacket();
    if (packet_size > 0) {
        uint8_t rx_buf[rudp::Endpoint::kMaxFrame];
        const int len = g_udp.read(rx_buf, sizeof(rx_buf));
        if (len > 0) {
            g_endpoint.OnUdpPacket(rx_buf, static_cast<uint16_t>(len));
        }
    }

    const uint32_t now = millis();
    if (now - g_last_tick_ms >= 10) {
        g_endpoint.Tick();
        g_last_tick_ms = now;
        LogStateChange();

        static uint32_t last_send_ms = 0;
        if (g_endpoint.IsConnected() && now - last_send_ms >= 30000) {
            SendSensorData();
            last_send_ms = now;
        }
    }

    static uint32_t last_stats_ms = 0;
    if (now - last_stats_ms >= 10000) {
        const rudp::RuntimeMetrics metrics = g_endpoint.GetRuntimeMetrics();
        Serial.print("[STATS] tx=");
        Serial.print(g_send_count);
        Serial.print(" rx=");
        Serial.print(g_recv_count);
        Serial.print(" pending=");
        Serial.print(metrics.pending_send);
        Serial.print(" rtt=");
        Serial.println(metrics.smoothed_rtt_ms);
        last_stats_ms = now;
    }

    delay(1);
}
