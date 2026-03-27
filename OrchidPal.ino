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

String SERIAL_NUMBER = "OP-0S-0R-LBUJKQ-006";

// Cấu trúc các Topic MQTT 
String TOPIC_PUB_STATUS    = "device/" + SERIAL_NUMBER + "/status";    
String TOPIC_SUB_MESSAGE   = "device/" + SERIAL_NUMBER + "/message";   
String TOPIC_SUB_PREFIX    = "device/" + SERIAL_NUMBER + "/control/";  
String TOPIC_SUB_RELAYS    = "device/" + SERIAL_NUMBER + "/control/#"; 
String TOPIC_PUB_TELEMETRY = "device/" + SERIAL_NUMBER + "/telemetry"; 
String TOPIC_SUB_CONFIG    = "device/" + SERIAL_NUMBER + "/config";    
String TOPIC_PUB_LOG       = "device/" + SERIAL_NUMBER + "/action_log";

TFT_eSPI tft = TFT_eSPI(); 
SemaphoreHandle_t uiMutex = NULL; 
Preferences sysPrefs;

// --- BIẾN LƯU TRẠNG THÁI PHẦN CỨNG ---
bool hasTemp   = false;
bool hasHumid  = false; 
bool hasSoil   = false; 
bool hasLight  = false;

bool hasRelay1 = false;
bool hasRelay2 = false;
bool hasRelay3 = false; 
bool hasRelay4 = false;

String roleRelay1 = "null";
String roleRelay2 = "null";
String roleRelay3 = "null";
String roleRelay4 = "null";

// --- BIẾN LOGIC AUTOMATION (RULE ENGINE) ---
LogicRule rules[MAX_RULES]; 
int ruleCount = 0;          

// Mảng 4 phần tử lưu trạng thái hẹn giờ cho 4 Relay
ActionState relayStates[4] = {
    {false, 0, 0, 0}, // Relay 1
    {false, 0, 0, 0}, // Relay 2
    {false, 0, 0, 0}, // Relay 3
    {false, 0, 0, 0}  // Relay 4
};

// --- CÁC BỘ ĐẾM THỜI GIAN NGẦM ---
unsigned long lastSensorTime = 0;    
unsigned long lastScrollTime = 0;    
unsigned long lastTelemetryTime = 0; 

TaskHandle_t NetworkTask;


// =========================================================================
// PHẦN 2: CÁC HÀM TẢI CẤU HÌNH TỪ BỘ NHỚ FLASH
// =========================================================================

void loadHardwareConfig() {
    sysPrefs.begin("hardware", true); 
    
    hasTemp  = sysPrefs.getBool("hasTemp", false);
    hasHumid = sysPrefs.getBool("hasHumid", false);
    hasSoil  = sysPrefs.getBool("hasSoil", false);
    hasLight = sysPrefs.getBool("hasLight", false);
    
    roleRelay1 = sysPrefs.getString("roleR1", "null");
    roleRelay2 = sysPrefs.getString("roleR2", "null");
    roleRelay3 = sysPrefs.getString("roleR3", "null");
    roleRelay4 = sysPrefs.getString("roleR4", "null");

    hasRelay1 = (roleRelay1 != "null");
    hasRelay2 = (roleRelay2 != "null");
    hasRelay3 = (roleRelay3 != "null");
    hasRelay4 = (roleRelay4 != "null");

    sysPrefs.end(); 
    Serial.println(">> Đã tải cấu hình phần cứng!");
}

void loadLogicConfig() {
    sysPrefs.begin("logic", true); 
    String logicStr = sysPrefs.getString("rules", "[]"); 
    sysPrefs.end();

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, logicStr); 
    
    if (!error) { 
        JsonArray arr = doc.as<JsonArray>(); 
        ruleCount = 0; 
        
        for (JsonObject rule : arr) {
            if (ruleCount >= MAX_RULES) break; 
            
            rules[ruleCount].metric      = rule["if"]["metric"].as<String>();
            rules[ruleCount].op          = rule["if"]["op"].as<String>();
            rules[ruleCount].value       = rule["if"]["value"].as<float>();
            rules[ruleCount].action      = rule["then"]["action"].as<String>();
            rules[ruleCount].duration_ms = rule["then"]["duration_ms"].as<unsigned long>();
            rules[ruleCount].cooldown_ms = rule["then"]["cooldown_ms"].as<unsigned long>() | 15000;
            
            ruleCount++; 
        }
        Serial.println(">> Đã tải " + String(ruleCount) + " quy tắc tự động!");
    }
}


// =========================================================================
// PHẦN 3: BỘ NÃO TỰ ĐỘNG HÓA (RULE ENGINE & GỬI LOG)
// =========================================================================

extern void publishActionLog(int relayNum, String role, String action, String reason, float value);

void activateRelayTimer(int relayNum, String role, unsigned long duration, unsigned long cooldown_ms, String metric, String op, float ruleValue, float currentValue) {
    int idx = relayNum - 1; 
    unsigned long now = millis();
    
    if (!relayStates[idx].isActive && (now - relayStates[idx].lastOffTime >= cooldown_ms)) {
        relayStates[idx].isActive = true;            
        relayStates[idx].startTime = now;       
        relayStates[idx].duration = duration;        
        
        setRelay(relayNum, true); 
        setFooterMsg("AUTO: R" + String(relayNum) + " ON (" + String(duration/1000) + "s)"); 
        Serial.println(">> Đã bật Relay " + String(relayNum) + " trong " + String(duration) + "ms.");

        // --- GỬI LOG LÊN SERVER ---
        String triggerReason = metric + " " + op + " " + String(ruleValue);
        publishActionLog(relayNum, role, "ON", triggerReason, currentValue);
    }
}

void triggerActionByRole(String targetRole, unsigned long duration, unsigned long cooldown_ms, String metric, String op, float ruleValue, float currentValue) {
    if (roleRelay1 == targetRole) activateRelayTimer(1, roleRelay1, duration, cooldown_ms, metric, op, ruleValue, currentValue);
    if (roleRelay2 == targetRole) activateRelayTimer(2, roleRelay2, duration, cooldown_ms, metric, op, ruleValue, currentValue);
    if (roleRelay3 == targetRole) activateRelayTimer(3, roleRelay3, duration, cooldown_ms, metric, op, ruleValue, currentValue);
    if (roleRelay4 == targetRole) activateRelayTimer(4, roleRelay4, duration, cooldown_ms, metric, op, ruleValue, currentValue);
}

void evaluateRules(SensorData data) {
    for (int i = 0; i < ruleCount; i++) { 
        float currentValue = 0;
        
        if (rules[i].metric == "temperature") currentValue = data.temp;
        else if (rules[i].metric == "humidity") currentValue = data.humid;
        else if (rules[i].metric == "soil_moisture") currentValue = data.soil;
        else if (rules[i].metric == "light") currentValue = data.light;

        bool conditionMet = false; 
        
        if (rules[i].op == ">" && currentValue > rules[i].value) conditionMet = true;
        else if (rules[i].op == "<" && currentValue < rules[i].value) conditionMet = true;
        else if (rules[i].op == "==" && currentValue == rules[i].value) conditionMet = true;

        if (conditionMet) {
            triggerActionByRole(rules[i].action, rules[i].duration_ms, rules[i].cooldown_ms, rules[i].metric, rules[i].op, rules[i].value, currentValue);
        }
    }
}

void loopAutoActions() {
    unsigned long now = millis(); 
    
    for (int i = 0; i < 4; i++) { 
        if (relayStates[i].isActive && (now - relayStates[i].startTime >= relayStates[i].duration)) {
            relayStates[i].isActive = false; 
            relayStates[i].lastOffTime = now; 
            
            int relayNum = i + 1; 
            setRelay(relayNum, false); 
            Serial.println("AUTO: Relay " + String(relayNum) + " OFF (Cooldown started)");
        }
    }
}


// =========================================================================
// PHẦN 4: HỆ ĐIỀU HÀNH ĐA LUỒNG (FREERTOS)
// =========================================================================
void networkTaskCode(void * pvParameters) {
    for(;;) { 
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

    loadHardwareConfig();
    loadLogicConfig();

    tft.init(); 
    tft.setRotation(0);      
    tft.fillScreen(COL_BG);  
    initSprites();           
    updateHeader(false, false); 
    
    updateSensors(0, 0, 0, 0); 
    updateRelays(false, false, false, false); // Tránh lỗi màn hình xanh lúc khởi động
    
    initRelays();  
    initSensors(); 
    
    // Ép tắt toàn bộ phần cứng và reset bộ đếm khi khởi động
    for (int i = 0; i < 4; i++) {
        relayStates[i].isActive = false;
        relayStates[i].startTime = 0;
        relayStates[i].duration = 0;
        relayStates[i].lastOffTime = millis();
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

    // [1] BỘ BẢO VỆ & HẸN GIỜ (Luôn chạy)
    loopAutoActions();

    // [2] NÚT BẤM RESET
    if (digitalRead(PIN_BUTTON_RESET) == LOW) { 
        delay(50); 
        if (digitalRead(PIN_BUTTON_RESET) == LOW) { 
            unsigned long pressTime = millis();     
            bool isTriggered = false;               
            
            while(digitalRead(PIN_BUTTON_RESET) == LOW) {
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
