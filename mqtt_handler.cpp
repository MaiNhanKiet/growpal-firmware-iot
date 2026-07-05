#include "mqtt_handler.h"
#include "config.h"
#include "ui_draw.h"
#include "relays.h"
#include "wifi_setup.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastReconnect = 0;
volatile bool mqttForceReconnect = false;

/** Sau N lần connect fail liên tiếp (~60s) → mở AP portal để sửa MQTT. */
static const int MQTT_FAIL_PORTAL_THRESHOLD = 12;
static int mqttFailStreak = 0;

extern void loadHardwareConfig();
extern void loadLogicConfig();

// ---------------------------------------------------------------------------
// Core 0: callback chỉ copy payload vào queue — không parse JSON / ghi Flash
// ---------------------------------------------------------------------------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (length == 0 || mqttQueue == NULL) return;

    // Static buffer: tránh chiếm ~2KB stack trên NetworkTask (Core 0)
    static MqttMessage msg;
    memset(&msg, 0, sizeof(msg));

    strncpy(msg.topic, topic, sizeof(msg.topic) - 1);
    msg.topic[sizeof(msg.topic) - 1] = '\0';

    if (length >= MQTT_PAYLOAD_MAX) {
        length = MQTT_PAYLOAD_MAX - 1;
        Serial.println("WARN: MQTT payload truncated to fit queue buffer");
    }
    memcpy(msg.payload, payload, length);
    msg.payload[length] = '\0';
    msg.length = (uint16_t)length;

    // Non-blocking: không làm chậm luồng mạng nếu queue đầy
    if (xQueueSend(mqttQueue, &msg, 0) != pdTRUE) {
        Serial.println("WARN: mqttQueue full — message dropped");
    }
}

// ---------------------------------------------------------------------------
// Core 1: xử lý cấu hình (JSON + Preferences + hot-reload UI)
// ---------------------------------------------------------------------------
static void handleConfigMessage(const MqttMessage* msg) {
    Serial.println(">> Dang phan tich file cau hinh moi...");

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, msg->payload, msg->length);

    if (error) {
        Serial.print("Loi doc JSON: ");
        Serial.println(error.c_str());
        return;
    }

    JsonObject dataObj = doc["data"];
    if (dataObj.isNull()) {
        Serial.println("Loi: Khong tim thay object 'data' tu NestJS!");
        return;
    }

    Preferences p;
    p.begin("hardware", false);

    JsonObject hw = dataObj["hardware"];
    p.putBool("hasTemp",  hw["sensors"]["temperature"] | false);
    p.putBool("hasHumid", hw["sensors"]["humidity"] | false);
    p.putBool("hasSoil",  hw["sensors"]["soil_moisture"] | false);
    p.putBool("hasLight", hw["sensors"]["light"] | false);

    p.putString("roleR1", hw["relays"]["relay_1"] | "null");
    p.putString("roleR2", hw["relays"]["relay_2"] | "null");
    p.putString("roleR3", hw["relays"]["relay_3"] | "null");
    p.putString("roleR4", hw["relays"]["relay_4"] | "null");
    p.end();

    Preferences pLogic;
    pLogic.begin("logic", false);

    JsonArray logicArr = dataObj["logic"].as<JsonArray>();
    char logicStr[1536];
    serializeJson(logicArr, logicStr, sizeof(logicStr));
    pLogic.putString("rules", logicStr);
    pLogic.end();

    loadHardwareConfig();
    loadLogicConfig();

    for (int i = 0; i < 4; i++) {
        sysState.relayStates[i].isActive = false;
        sysState.relayStates[i].startTime = 0;
        sysState.relayStates[i].duration = 0;
        sysState.relayStates[i].lastOffTime = millis();
        setRelay(i + 1, false);
    }

    if (xSemaphoreTake(uiMutex, portMAX_DELAY)) {
        tft.fillScreen(COL_BG);
        xSemaphoreGive(uiMutex);
    }
    updateHeader(true, true);
    updateSensors(0, 0, 0, 0);
    updateRelays(false, false, false, false);

    setFooterMsg("Hot-Reload: Config Applied!");
    Serial.println(">> Da cap nhat cau hinh nong thanh cong!");

    client.publish(TOPIC_SUB_CONFIG, "", true);
}

// ---------------------------------------------------------------------------
// Core 1: lệnh message / relay thủ công
// ---------------------------------------------------------------------------
static void handleMessageTopic(const MqttMessage* msg) {
    char footerBuf[96];
    snprintf(footerBuf, sizeof(footerBuf), "Server: %s", msg->payload);
    setFooterMsg(footerBuf);
}

static bool payloadMeansOn(const char* payload) {
    return (strstr(payload, "\"ON\"") != NULL ||
            strstr(payload, "\"true\"") != NULL ||
            strstr(payload, "\"1\"") != NULL ||
            strstr(payload, "ON") != NULL);
}

static void handleRelayCommand(const MqttMessage* msg) {
    bool state = payloadMeansOn(msg->payload);

    size_t topicLen = strlen(msg->topic);
    if (topicLen == 0) return;

    char relayIdChar = msg->topic[topicLen - 1];
    int relayNum = relayIdChar - '0';

    if (relayNum < 1 || relayNum > 4) return;

    bool isConfigured = false;
    if (relayNum == 1) isConfigured = sysState.hasRelay1;
    else if (relayNum == 2) isConfigured = sysState.hasRelay2;
    else if (relayNum == 3) isConfigured = sysState.hasRelay3;
    else if (relayNum == 4) isConfigured = sysState.hasRelay4;

    char footerBuf[48];
    if (!isConfigured) {
        Serial.printf("Mobile: TU CHOI! Relay %d chua duoc cau hinh.\n", relayNum);
        snprintf(footerBuf, sizeof(footerBuf), "Error: R%d unconfigured!", relayNum);
        setFooterMsg(footerBuf);
        return;
    }

    snprintf(footerBuf, sizeof(footerBuf), "Mobile: Relay %d > Turn %s",
             relayNum, state ? "On" : "Off");
    setFooterMsg(footerBuf);
    setRelay(relayNum, state);

    int idx = relayNum - 1;
    sysState.relayStates[idx].isActive = false;
    if (!state) {
        sysState.relayStates[idx].lastOffTime = millis();
    }
    Serial.printf("Mobile: Relay %d > Turn %s\n", relayNum, state ? "ON" : "OFF");
}

void processMqttMessage(const MqttMessage* msg) {
    if (msg == NULL) return;

    if (strcmp(msg->topic, TOPIC_SUB_CONFIG) == 0) {
        handleConfigMessage(msg);
        return;
    }

    if (strcmp(msg->topic, TOPIC_SUB_MESSAGE) == 0) {
        handleMessageTopic(msg);
        return;
    }

    if (strncmp(msg->topic, TOPIC_SUB_PREFIX, strlen(TOPIC_SUB_PREFIX)) == 0) {
        handleRelayCommand(msg);
    }
}

// --- KẾT NỐI BROKER (credentials từ Flash / Captive Portal) ---
void connectMqtt() {
    if (WiFi.status() != WL_CONNECTED) {
        updateHeader(false, false);
        return;
    }

    if (mqttForceReconnect) {
        mqttForceReconnect = false;
        if (client.connected()) client.disconnect();
        mqttFailStreak = 0;
        lastReconnect = 0;
        Serial.println(">> MQTT force reconnect (new credentials)");
    }

    if (client.connected()) return;

    if (mqttServer[0] == '\0') {
        static unsigned long lastWarn = 0;
        if (millis() - lastWarn > 15000) {
            lastWarn = millis();
            setFooterMsg("MQTT: Configure broker in Setup Portal");
            Serial.println("WARN: mqttServer empty — skip connect");
            openConfigPortal("MQTT not set!");
        }
        updateHeader(true, false);
        return;
    }

    if (millis() - lastReconnect > 5000) {
        lastReconnect = millis();
        setFooterMsg("Connecting to Server...");

        client.setServer(mqttServer, mqttPort);

        char clientId[48];
        snprintf(clientId, sizeof(clientId), "GrowPal_%s", SERIAL_NUMBER);
        const char* willPayload = "{\"status\":\"OFFLINE\"}";

        bool ok;
        if (mqttUser[0] != '\0') {
            ok = client.connect(clientId, mqttUser, mqttPass,
                                TOPIC_PUB_STATUS, 1, true, willPayload);
        } else {
            ok = client.connect(clientId, TOPIC_PUB_STATUS, 1, true, willPayload);
        }

        if (ok) {
            mqttFailStreak = 0;
            setFooterMsg("System Ready | Server Online");
            updateHeader(true, true);

            client.publish(TOPIC_PUB_STATUS, "{\"status\":\"ONLINE\"}", true);

            client.subscribe(TOPIC_SUB_RELAYS);
            client.subscribe(TOPIC_SUB_MESSAGE);
            client.subscribe(TOPIC_SUB_CONFIG);
        } else {
            updateHeader(true, false);
            char errBuf[32];
            snprintf(errBuf, sizeof(errBuf), "MQTT Error: %d", client.state());
            setFooterMsg(errBuf);

            mqttFailStreak++;
            if (mqttFailStreak >= MQTT_FAIL_PORTAL_THRESHOLD) {
                mqttFailStreak = 0;
                openConfigPortal("MQTT offline!");
            }
        }
    }
}

void initMqtt() {
    if (mqttServer[0] != '\0') {
        client.setServer(mqttServer, mqttPort);
    }
    client.setCallback(mqttCallback);
    client.setBufferSize(2048);
    client.setKeepAlive(5);
}

void loopMqtt() {
    static bool lastWifiStatus = false;
    static bool lastMqttStatus = false;

    bool currentWifiStatus = (WiFi.status() == WL_CONNECTED);

    if (mqttForceReconnect && client.connected()) {
        client.disconnect();
    }

    if (currentWifiStatus) {
        if (!client.connected()) connectMqtt();
        else client.loop();
    } else {
        mqttFailStreak = 0;
    }

    // Mất MQTT (từng online → offline) trong khi WiFi còn: mở portal sau vài chu kỳ fail
    bool currentMqttStatus = client.connected();
    if (lastMqttStatus && !currentMqttStatus && currentWifiStatus) {
        Serial.println("MQTT Connection Lost!");
        // Không mở portal ngay — để connectMqtt() đếm fail streak
    }

    if (currentWifiStatus != lastWifiStatus || currentMqttStatus != lastMqttStatus) {
        updateHeader(currentWifiStatus, currentMqttStatus);
        lastWifiStatus = currentWifiStatus;
        lastMqttStatus = currentMqttStatus;
    }
}

// --- GỬI TELEMETRY (static char buffer, no String concat) ---
void publishSensorData(float t, float h, int s, int l) {
    if (!client.connected()) return;

    if (!sysState.hasTemp && !sysState.hasHumid &&
        !sysState.hasSoil && !sysState.hasLight) {
        Serial.println(">> Bo qua Telemetry: Mach chua duoc cau hinh cam bien!");
        return;
    }

    char payload[128];
    size_t offset = 0;
    payload[offset++] = '{';

    bool first = true;
    int written = 0;

    if (sysState.hasTemp) {
        written = snprintf(payload + offset, sizeof(payload) - offset,
                           "\"temp\":%.1f", t);
        if (written > 0) offset += (size_t)written;
        first = false;
    }
    if (sysState.hasHumid) {
        written = snprintf(payload + offset, sizeof(payload) - offset,
                           "%s\"humid\":%.1f", first ? "" : ",", h);
        if (written > 0) offset += (size_t)written;
        first = false;
    }
    if (sysState.hasSoil) {
        written = snprintf(payload + offset, sizeof(payload) - offset,
                           "%s\"soil\":%d", first ? "" : ",", s);
        if (written > 0) offset += (size_t)written;
        first = false;
    }
    if (sysState.hasLight) {
        written = snprintf(payload + offset, sizeof(payload) - offset,
                           "%s\"light\":%d", first ? "" : ",", l);
        if (written > 0) offset += (size_t)written;
    }

    if (offset < sizeof(payload) - 1) {
        payload[offset++] = '}';
        payload[offset] = '\0';
    } else {
        payload[sizeof(payload) - 2] = '}';
        payload[sizeof(payload) - 1] = '\0';
    }

    client.publish(TOPIC_PUB_TELEMETRY, payload);
}

// --- GỬI LOG AUTOMATION ---
void publishActionLog(int relayNum, const char* role, const char* action,
                      const char* reason, float value) {
    if (!client.connected()) return;

    char payload[256];
    snprintf(payload, sizeof(payload),
             "{\"action\":\"%s\",\"relay_number\":%d,\"role\":\"%s\","
             "\"trigger_reason\":\"%s\",\"value\":%.1f}",
             action ? action : "",
             relayNum,
             role ? role : "",
             reason ? reason : "",
             value);

    client.publish(TOPIC_PUB_LOG, payload);
    Serial.print(">> Da gui Log Su kien len Server: ");
    Serial.println(payload);
}
