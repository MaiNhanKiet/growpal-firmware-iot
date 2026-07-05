#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// --- 1. MQTT CREDENTIAL BUFFER SIZES (values live in Flash / RAM, not macros) ---
#define MQTT_HOST_LEN       64
#define MQTT_USER_LEN       32
#define MQTT_PASS_LEN       64
#define MQTT_PORT_DEFAULT   1883
/** Factory defaults — kept when user only configures WiFi and never saves MQTT. */
#define MQTT_DEFAULT_SERVER "167.71.201.144"
#define MQTT_DEFAULT_USER   "admin"
#define MQTT_DEFAULT_PASS   "123456"

extern char     mqttServer[MQTT_HOST_LEN];
extern uint16_t mqttPort;
extern char     mqttUser[MQTT_USER_LEN];
extern char     mqttPass[MQTT_PASS_LEN];

/** Set by portal Save MQTT — mqtt_handler disconnects and reconnects with new creds. */
extern volatile bool mqttForceReconnect;

// --- 2. PIN DEFINITIONS ---
#define PIN_RELAY_1     4
#define PIN_RELAY_2     5
#define PIN_RELAY_3     15
#define PIN_RELAY_4     16
#define PIN_BUTTON_RESET 0

// --- 3. BUFFER / QUEUE SIZES ---
#define MAX_RULES           10
#define SERIAL_NUMBER_LEN   32
#define TOPIC_LEN           64
#define ROLE_LEN            32
#define RULE_METRIC_LEN     32
#define RULE_OP_LEN         4
#define RULE_ACTION_LEN     32
#define MQTT_TOPIC_MAX      96
#define MQTT_PAYLOAD_MAX    2048
#define MQTT_QUEUE_LEN      5

// --- 4. HARDWARE OBJECT ---
extern TFT_eSPI tft;

// --- 5. STATIC IDENTITY & MQTT TOPICS (no heap String) ---
extern char SERIAL_NUMBER[SERIAL_NUMBER_LEN];
extern char TOPIC_SUB_PREFIX[TOPIC_LEN];
extern char TOPIC_SUB_RELAYS[TOPIC_LEN];
extern char TOPIC_SUB_MESSAGE[TOPIC_LEN];
extern char TOPIC_PUB_STATUS[TOPIC_LEN];
extern char TOPIC_PUB_TELEMETRY[TOPIC_LEN];
extern char TOPIC_SUB_CONFIG[TOPIC_LEN];
extern char TOPIC_PUB_LOG[TOPIC_LEN];

// --- 6. LOGIC AUTOMATION TYPES ---
struct LogicRule {
    char metric[RULE_METRIC_LEN];
    char op[RULE_OP_LEN];
    float value;
    char action[RULE_ACTION_LEN];
    unsigned long duration_ms;
    unsigned long cooldown_ms;
};

struct ActionState {
    bool isActive;
    unsigned long startTime;
    unsigned long duration;
    unsigned long lastOffTime;
};

// --- 7. CENTRALIZED SYSTEM STATE ---
struct SystemState {
    bool hasTemp;
    bool hasHumid;
    bool hasSoil;
    bool hasLight;

    bool hasRelay1;
    bool hasRelay2;
    bool hasRelay3;
    bool hasRelay4;

    char roleRelay1[ROLE_LEN];
    char roleRelay2[ROLE_LEN];
    char roleRelay3[ROLE_LEN];
    char roleRelay4[ROLE_LEN];

    LogicRule rules[MAX_RULES];
    int ruleCount;
    ActionState relayStates[4];
};

extern SystemState sysState;

// --- 8. MQTT FREE RTOS QUEUE (Core 0 enqueue / Core 1 dequeue) ---
struct MqttMessage {
    char topic[MQTT_TOPIC_MAX];
    char payload[MQTT_PAYLOAD_MAX];
    uint16_t length;
};

extern QueueHandle_t mqttQueue;

#endif
