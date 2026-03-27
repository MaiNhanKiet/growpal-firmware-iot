#include "mqtt_handler.h"
#include "config.h"
#include "ui_draw.h"
#include "relays.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastReconnect = 0;

// Các biến ngoại lai từ file main
extern void loadHardwareConfig();
extern void loadLogicConfig();
extern TFT_eSPI tft;
extern ActionState relayStates[4]; 
extern String TOPIC_PUB_LOG;

void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Chốt chặn an toàn: Bỏ qua payload rỗng
    if (length == 0) return;

    // --- ĐỌC CHUỖI DÀI KHÔNG GÂY TRÀN RAM ---
    String msg = "";
    msg.reserve(length + 1);
    for (int i = 0; i < length; i++) msg += (char)payload[i];
    
    String topicStr = String(topic);

    // =========================================================
    // XỬ LÝ KHI NHẬN CẤU HÌNH TỪ BACKEND NESTJS
    // =========================================================
    if (topicStr == TOPIC_SUB_CONFIG) {
        Serial.println(">> Đang phân tích file cấu hình mới...");
        
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, payload, length);
        
        if (error) {
            Serial.print("Lỗi đọc JSON: ");
            Serial.println(error.c_str());
            return;
        }

        JsonObject dataObj = doc["data"];
        if (dataObj.isNull()) {
            Serial.println("Lỗi: Không tìm thấy object 'data' từ NestJS!");
            return;
        }

        // --- 1. LƯU CẤU HÌNH HARDWARE ---
        Preferences p;
        p.begin("hardware", false);
        
        JsonObject hw = dataObj["hardware"];
        p.putBool("hasTemp", hw["sensors"]["temperature"] | false);
        p.putBool("hasHumid", hw["sensors"]["humidity"] | false);
        p.putBool("hasSoil", hw["sensors"]["soil_moisture"] | false);
        p.putBool("hasLight", hw["sensors"]["light"] | false);

        p.putString("roleR1", hw["relays"]["relay_1"] | "null");
        p.putString("roleR2", hw["relays"]["relay_2"] | "null");
        p.putString("roleR3", hw["relays"]["relay_3"] | "null");
        p.putString("roleR4", hw["relays"]["relay_4"] | "null");
        p.end();

        // --- 2. LƯU CẤU HÌNH LOGIC AUTOMATION ---
        Preferences pLogic;
        pLogic.begin("logic", false);
        
        JsonArray logicArr = dataObj["logic"].as<JsonArray>();
        String logicStr;
        serializeJson(logicArr, logicStr);
        pLogic.putString("rules", logicStr);
        pLogic.end();

        // --- SỬA LỖI REBOOT VÔ HẠN: DÙNG HOT-RELOAD ---
        loadHardwareConfig();
        loadLogicConfig();
        
        // Ép tắt Relay và Reset Timer khi nhận Config mới
        for (int i = 0; i < 4; i++) {
            relayStates[i].isActive = false;      
            relayStates[i].startTime = 0;         
            relayStates[i].duration = 0;
            relayStates[i].lastOffTime = millis();
            setRelay(i + 1, false);               
        }

        tft.fillScreen(COL_BG);
        updateHeader(true, true); 
        updateSensors(0, 0, 0, 0); 
        updateRelays(false, false, false, false); // Sửa lỗi nút xanh

        setFooterMsg("Hot-Reload: Config Applied!");
        Serial.println(">> Đã cập nhật cấu hình nóng thành công!");

        // Gửi lệnh xóa Config cũ trên Broker
        client.publish(TOPIC_SUB_CONFIG.c_str(), "", true); 
        return;
    }

    // =========================================================
    // XỬ LÝ LỆNH MESSAGE
    // =========================================================
    if (topicStr == TOPIC_SUB_MESSAGE) {
        setFooterMsg("Server: " + msg);
        return; 
    }

    // =========================================================
    // XỬ LÝ LỆNH RELAY THỦ CÔNG (ĐỒNG BỘ AUTO & BẢO VỆ CẤU HÌNH)
    // =========================================================
    if (topicStr.startsWith(TOPIC_SUB_PREFIX)) {
        bool state = false;
        if (msg.indexOf("\"ON\"") != -1 || msg.indexOf("\"true\"") != -1 || msg.indexOf("\"1\"") != -1 || msg.indexOf("ON") != -1) {
            state = true;
        }

        char relayIdChar = topicStr.charAt(topicStr.length() - 1);
        int relayNum = String(relayIdChar).toInt(); 

        if (relayNum >= 1 && relayNum <= 4) {
            bool isConfigured = false;
            if (relayNum == 1) isConfigured = hasRelay1;
            else if (relayNum == 2) isConfigured = hasRelay2;
            else if (relayNum == 3) isConfigured = hasRelay3;
            else if (relayNum == 4) isConfigured = hasRelay4;

            if (!isConfigured) {
                Serial.println("Mobile: TỪ CHỐI! Relay " + String(relayNum) + " chưa được cấu hình.");
                setFooterMsg("Error: R" + String(relayNum) + " unconfigured!");
                return;
            }

            setFooterMsg("Mobile: Relay " + String(relayNum) + " > Turn " + (state ? "On" : "Off"));
            setRelay(relayNum, state);
            
            int idx = relayNum - 1; 
            relayStates[idx].isActive = false; 
            
            if (!state) {
                relayStates[idx].lastOffTime = millis();
            }
            Serial.println("Mobile: Relay " + String(relayNum) + " > Turn " + (state ? "ON" : "OFF"));
        }
    }
}

// --- KẾT NỐI BROKER ---
void connectMqtt() {
    if (WiFi.status() != WL_CONNECTED) {
        updateHeader(false, false);
        return; 
    }

    if (client.connected()) return;

    if (millis() - lastReconnect > 5000) {
        lastReconnect = millis();
        setFooterMsg("Connecting to Server..."); 
        
        String clientId = "OrchidPal_" + SERIAL_NUMBER;
        const char* willTopic = TOPIC_PUB_STATUS.c_str();
        const char* willPayload = "{\"status\":\"OFFLINE\"}";

        if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASS, willTopic, 1, true, willPayload)) {
            setFooterMsg("System Ready | Server Online");
            updateHeader(true, true); 
            
            client.publish(TOPIC_PUB_STATUS.c_str(), "{\"status\":\"ONLINE\"}", true);

            client.subscribe(TOPIC_SUB_RELAYS.c_str());  
            client.subscribe(TOPIC_SUB_MESSAGE.c_str()); 
            client.subscribe(TOPIC_SUB_CONFIG.c_str());
            
        } else {
            updateHeader(true, false);
            setFooterMsg("MQTT Error: " + String(client.state()));
        }
    }
}

void initMqtt() {
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(mqttCallback);
    client.setBufferSize(2048);
    client.setKeepAlive(5);
}

void loopMqtt() {
    static bool lastWifiStatus = false;
    static bool lastMqttStatus = false;

    bool currentWifiStatus = (WiFi.status() == WL_CONNECTED);
    
    if (currentWifiStatus) {
        if (!client.connected()) connectMqtt();
        else client.loop();
    }

    bool currentMqttStatus = client.connected();

    if (currentWifiStatus != lastWifiStatus || currentMqttStatus != lastMqttStatus) {
        updateHeader(currentWifiStatus, currentMqttStatus);
        lastWifiStatus = currentWifiStatus;
        lastMqttStatus = currentMqttStatus;
    }
}

// --- GỬI TELEMETRY ---
void publishSensorData(float t, float h, int s, int l) {
    if (!client.connected()) return;

    if (!hasTemp && !hasHumid && !hasSoil && !hasLight) {
        Serial.println(">> Bỏ qua Telemetry: Mạch chưa được cấu hình cảm biến!");
        return;
    }

    String payload = "{";
    bool first = true;
    if (hasTemp) { payload += "\"temp\":" + String(t, 1); first = false; }
    if (hasHumid) { if(!first) payload += ","; payload += "\"humid\":" + String(h, 1); first = false; }
    if (hasSoil) { if(!first) payload += ","; payload += "\"soil\":" + String(s); first = false; }
    if (hasLight) { if(!first) payload += ","; payload += "\"light\":" + String(l); }
    payload += "}";
    
    client.publish(TOPIC_PUB_TELEMETRY.c_str(), payload.c_str());
}

// --- GỬI LOG AUTOMATION ---
void publishActionLog(int relayNum, String role, String action, String reason, float value) {
    if (!client.connected()) return;

    StaticJsonDocument<256> doc;
    doc["action"] = action;              
    doc["relay_number"] = relayNum;      
    doc["role"] = role;                  
    doc["trigger_reason"] = reason;      
    doc["value"] = value;                

    String payloadStr;
    serializeJson(doc, payloadStr);

    client.publish(TOPIC_PUB_LOG.c_str(), payloadStr.c_str());
    Serial.println(">> Đã gửi Log Sự kiện lên Server: " + payloadStr);
}
