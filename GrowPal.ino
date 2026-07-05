// =========================================================================
// PHẦN 1: KHAI BÁO THƯ VIỆN & CÁC BIẾN TOÀN CỤC
// =========================================================================
#include "config.h"
#include "ui_draw.h"
#include "wifi_setup.h"
#include "mqtt_handler.h"
#include "sensor.h"
#include "relays.h"
#include <Preferences.h>
#include <ArduinoJson.h>

char SERIAL_NUMBER[SERIAL_NUMBER_LEN] = "GP-0S-0R-LBUJKQ-006";

char TOPIC_PUB_STATUS[TOPIC_LEN];
char TOPIC_SUB_MESSAGE[TOPIC_LEN];
char TOPIC_SUB_PREFIX[TOPIC_LEN];
char TOPIC_SUB_RELAYS[TOPIC_LEN];
char TOPIC_PUB_TELEMETRY[TOPIC_LEN];
char TOPIC_SUB_CONFIG[TOPIC_LEN];
char TOPIC_PUB_LOG[TOPIC_LEN];

TFT_eSPI tft = TFT_eSPI();
SemaphoreHandle_t uiMutex = NULL;
QueueHandle_t mqttQueue = NULL;
Preferences sysPrefs;

SystemState sysState = {
    false, false, false, false,
    false, false, false, false,
    "null", "null", "null", "null",
    {},
    0,
    {
        {false, 0, 0, 0},
        {false, 0, 0, 0},
        {false, 0, 0, 0},
        {false, 0, 0, 0}
    }
};

unsigned long lastSensorTime = 0;
unsigned long lastScrollTime = 0;
unsigned long lastTelemetryTime = 0;

TaskHandle_t NetworkTask;


// =========================================================================
// PHẦN 2: CÁC HÀM TẢI CẤU HÌNH TỪ BỘ NHỚ FLASH
// =========================================================================

static void loadRolePref(Preferences& prefs, const char* key, char* dest, size_t destLen) {
    if (prefs.getString(key, dest, destLen) == 0) {
        strncpy(dest, "null", destLen - 1);
        dest[destLen - 1] = '\0';
    }
}

void loadHardwareConfig() {
    sysPrefs.begin("hardware", true);

    sysState.hasTemp  = sysPrefs.getBool("hasTemp", false);
    sysState.hasHumid = sysPrefs.getBool("hasHumid", false);
    sysState.hasSoil  = sysPrefs.getBool("hasSoil", false);
    sysState.hasLight = sysPrefs.getBool("hasLight", false);

    loadRolePref(sysPrefs, "roleR1", sysState.roleRelay1, sizeof(sysState.roleRelay1));
    loadRolePref(sysPrefs, "roleR2", sysState.roleRelay2, sizeof(sysState.roleRelay2));
    loadRolePref(sysPrefs, "roleR3", sysState.roleRelay3, sizeof(sysState.roleRelay3));
    loadRolePref(sysPrefs, "roleR4", sysState.roleRelay4, sizeof(sysState.roleRelay4));

    sysState.hasRelay1 = (strcmp(sysState.roleRelay1, "null") != 0);
    sysState.hasRelay2 = (strcmp(sysState.roleRelay2, "null") != 0);
    sysState.hasRelay3 = (strcmp(sysState.roleRelay3, "null") != 0);
    sysState.hasRelay4 = (strcmp(sysState.roleRelay4, "null") != 0);

    sysPrefs.end();
    Serial.println(">> Da tai cau hinh phan cung!");
}

void loadLogicConfig() {
    sysPrefs.begin("logic", true);

    char logicBuf[2048];
    size_t len = sysPrefs.getString("rules", logicBuf, sizeof(logicBuf));
    sysPrefs.end();

    if (len == 0) {
        strncpy(logicBuf, "[]", sizeof(logicBuf) - 1);
        logicBuf[sizeof(logicBuf) - 1] = '\0';
    }

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, logicBuf);

    if (!error) {
        JsonArray arr = doc.as<JsonArray>();
        sysState.ruleCount = 0;

        for (JsonObject rule : arr) {
            if (sysState.ruleCount >= MAX_RULES) break;

            LogicRule& r = sysState.rules[sysState.ruleCount];
            const char* metric = rule["if"]["metric"] | "";
            const char* op     = rule["if"]["op"] | "";
            const char* action = rule["then"]["action"] | "";

            strncpy(r.metric, metric, sizeof(r.metric) - 1);
            r.metric[sizeof(r.metric) - 1] = '\0';
            strncpy(r.op, op, sizeof(r.op) - 1);
            r.op[sizeof(r.op) - 1] = '\0';
            strncpy(r.action, action, sizeof(r.action) - 1);
            r.action[sizeof(r.action) - 1] = '\0';

            r.value       = rule["if"]["value"].as<float>();
            r.duration_ms = rule["then"]["duration_ms"].as<unsigned long>();
            r.cooldown_ms = rule["then"]["cooldown_ms"] | 15000UL;

            sysState.ruleCount++;
        }
        Serial.printf(">> Da tai %d quy tac tu dong!\n", sysState.ruleCount);
    }
}


// =========================================================================
// PHẦN 3: BỘ NÃO TỰ ĐỘNG HÓA (RULE ENGINE & GỬI LOG)
// =========================================================================

void activateRelayTimer(int relayNum, const char* role, unsigned long duration,
                        unsigned long cooldown_ms, const char* metric, const char* op,
                        float ruleValue, float currentValue) {
    int idx = relayNum - 1;
    unsigned long now = millis();

    if (!sysState.relayStates[idx].isActive &&
        (now - sysState.relayStates[idx].lastOffTime >= cooldown_ms)) {
        sysState.relayStates[idx].isActive  = true;
        sysState.relayStates[idx].startTime = now;
        sysState.relayStates[idx].duration  = duration;

        setRelay(relayNum, true);

        char footerBuf[48];
        snprintf(footerBuf, sizeof(footerBuf), "AUTO: R%d ON (%lus)", relayNum, duration / 1000UL);
        setFooterMsg(footerBuf);
        Serial.printf(">> Da bat Relay %d trong %lu ms.\n", relayNum, duration);

        char triggerReason[64];
        snprintf(triggerReason, sizeof(triggerReason), "%s %s %.1f", metric, op, ruleValue);
        publishActionLog(relayNum, role, "ON", triggerReason, currentValue);
    }
}

void triggerActionByRole(const char* targetRole, unsigned long duration, unsigned long cooldown_ms,
                         const char* metric, const char* op, float ruleValue, float currentValue) {
    if (strcmp(sysState.roleRelay1, targetRole) == 0)
        activateRelayTimer(1, sysState.roleRelay1, duration, cooldown_ms, metric, op, ruleValue, currentValue);
    if (strcmp(sysState.roleRelay2, targetRole) == 0)
        activateRelayTimer(2, sysState.roleRelay2, duration, cooldown_ms, metric, op, ruleValue, currentValue);
    if (strcmp(sysState.roleRelay3, targetRole) == 0)
        activateRelayTimer(3, sysState.roleRelay3, duration, cooldown_ms, metric, op, ruleValue, currentValue);
    if (strcmp(sysState.roleRelay4, targetRole) == 0)
        activateRelayTimer(4, sysState.roleRelay4, duration, cooldown_ms, metric, op, ruleValue, currentValue);
}

void evaluateRules(SensorData data) {
    for (int i = 0; i < sysState.ruleCount; i++) {
        LogicRule& rule = sysState.rules[i];
        float currentValue = 0;

        if (strcmp(rule.metric, "temperature") == 0) currentValue = data.temp;
        else if (strcmp(rule.metric, "humidity") == 0) currentValue = data.humid;
        else if (strcmp(rule.metric, "soil_moisture") == 0) currentValue = data.soil;
        else if (strcmp(rule.metric, "light") == 0) currentValue = data.light;

        bool conditionMet = false;

        if (strcmp(rule.op, ">") == 0 && currentValue > rule.value) conditionMet = true;
        else if (strcmp(rule.op, "<") == 0 && currentValue < rule.value) conditionMet = true;
        else if (strcmp(rule.op, "==") == 0 && currentValue == rule.value) conditionMet = true;

        if (conditionMet) {
            triggerActionByRole(rule.action, rule.duration_ms, rule.cooldown_ms,
                                rule.metric, rule.op, rule.value, currentValue);
        }
    }
}

void loopAutoActions() {
    unsigned long now = millis();

    for (int i = 0; i < 4; i++) {
        if (sysState.relayStates[i].isActive &&
            (now - sysState.relayStates[i].startTime >= sysState.relayStates[i].duration)) {
            sysState.relayStates[i].isActive = false;
            sysState.relayStates[i].lastOffTime = now;

            int relayNum = i + 1;
            setRelay(relayNum, false);
            Serial.printf("AUTO: Relay %d OFF (Cooldown started)\n", relayNum);
        }
    }
}


// =========================================================================
// PHẦN 4: HỆ ĐIỀU HÀNH ĐA LUỒNG (FREERTOS)
// =========================================================================
void networkTaskCode(void * pvParameters) {
    for (;;) {
        loopWifi();
        loopMqtt();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}


// =========================================================================
// PHẦN 5: HÀM KHỞI TẠO SETUP (Core 1)
// =========================================================================
void setup() {
    Serial.begin(115200);
    pinMode(PIN_BUTTON_RESET, INPUT_PULLUP);

    uiMutex = xSemaphoreCreateMutex();
    mqttQueue = xQueueCreate(MQTT_QUEUE_LEN, sizeof(MqttMessage));
    if (mqttQueue == NULL) {
        Serial.println("ERROR: mqttQueue create failed - out of RAM!");
    }

    // Build MQTT topics once into static char buffers (no heap fragmentation)
    snprintf(TOPIC_PUB_STATUS,    sizeof(TOPIC_PUB_STATUS),    "device/%s/status",     SERIAL_NUMBER);
    snprintf(TOPIC_SUB_MESSAGE,   sizeof(TOPIC_SUB_MESSAGE),   "device/%s/message",    SERIAL_NUMBER);
    snprintf(TOPIC_SUB_PREFIX,    sizeof(TOPIC_SUB_PREFIX),    "device/%s/control/",   SERIAL_NUMBER);
    snprintf(TOPIC_SUB_RELAYS,    sizeof(TOPIC_SUB_RELAYS),    "device/%s/control/#",  SERIAL_NUMBER);
    snprintf(TOPIC_PUB_TELEMETRY, sizeof(TOPIC_PUB_TELEMETRY), "device/%s/telemetry",  SERIAL_NUMBER);
    snprintf(TOPIC_SUB_CONFIG,    sizeof(TOPIC_SUB_CONFIG),    "device/%s/config",     SERIAL_NUMBER);
    snprintf(TOPIC_PUB_LOG,       sizeof(TOPIC_PUB_LOG),       "device/%s/action_log", SERIAL_NUMBER);

    loadHardwareConfig();
    loadLogicConfig();

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(COL_BG);
    initSprites();
    updateHeader(false, false);

    updateSensors(0, 0, 0, 0);
    updateRelays(false, false, false, false);

    initRelays();
    initSensors();

    for (int i = 0; i < 4; i++) {
        sysState.relayStates[i].isActive = false;
        sysState.relayStates[i].startTime = 0;
        sysState.relayStates[i].duration = 0;
        sysState.relayStates[i].lastOffTime = millis();
        setRelay(i + 1, false);
    }

    initWifi();
    initMqtt();

    xTaskCreatePinnedToCore(
        networkTaskCode,
        "NetworkTask",
        10000,
        NULL,
        1,
        &NetworkTask,
        0
    );
}


// =========================================================================
// PHẦN 6: VÒNG LẶP CHÍNH (Core 1)
// =========================================================================
void loop() {
    unsigned long now = millis();

    // [0] XỬ LÝ MQTT QUEUE (JSON / Preferences / UI trên Core 1 — tránh WDT)
    if (mqttQueue != NULL) {
        static MqttMessage msg;
        while (xQueueReceive(mqttQueue, &msg, 0) == pdTRUE) {
            processMqttMessage(&msg);
        }
    }

    // [1] BỘ BẢO VỆ & HẸN GIỜ (Luôn chạy)
    loopAutoActions();

    // [2] NÚT BẤM RESET
    if (digitalRead(PIN_BUTTON_RESET) == LOW) {
        delay(50);
        if (digitalRead(PIN_BUTTON_RESET) == LOW) {
            unsigned long pressTime = millis();
            bool isTriggered = false;

            while (digitalRead(PIN_BUTTON_RESET) == LOW) {
                if (!isTriggered && (millis() - pressTime > 3000)) {
                    isTriggered = true;
                    setFooterMsg("RESETTING WIFI...");
                    if (xSemaphoreTake(uiMutex, portMAX_DELAY)) {
                        tft.fillScreen(TFT_RED);
                        xSemaphoreGive(uiMutex);
                    }
                }
                delay(10);
                loopAutoActions();
            }
            if (isTriggered) resetWifiConfig();
        }
    }

    // [3] ĐỌC CẢM BIẾN VÀ HIỂN THỊ (Mỗi 5s)
    if (now - lastSensorTime >= 5000) {
        lastSensorTime = now;

        SensorData data = readSensors();
        updateSensors(data.temp, data.humid, data.soil, data.light);

        // [4] BỘ NÃO LOGIC & TELEMETRY (Mỗi 60s)
        if (now - lastTelemetryTime >= 60000) {
            lastTelemetryTime = now;

            evaluateRules(data);
            publishSensorData(data.temp, data.humid, data.soil, data.light);
        }
    }

    // [5] HIỆU ỨNG GIAO DIỆN MƯỢT MÀ
    if (now - lastScrollTime >= 30) {
        lastScrollTime = now;
        scrollFooter();
    }
}
