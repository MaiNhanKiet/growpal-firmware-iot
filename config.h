#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <TFT_eSPI.h> 

// --- 1. WIFI ---
#define MQTT_SERVER     "167.71.201.144" 
#define MQTT_PORT       1883
#define MQTT_USER       "admin" 
#define MQTT_PASS       "123456"

// --- 2. PIN DEFINITIONS ---
#define PIN_RELAY_1     4
#define PIN_RELAY_2     5
#define PIN_RELAY_3     15
#define PIN_RELAY_4     16
#define PIN_BUTTON_RESET 0

// --- 3. EXTERN OBJECTS (HARDWARE) ---
extern String SERIAL_NUMBER;
extern TFT_eSPI tft; 

extern bool hasTemp;
extern bool hasHumid;
extern bool hasSoil;
extern bool hasLight;

extern bool hasRelay1;
extern bool hasRelay2;
extern bool hasRelay3;
extern bool hasRelay4;

extern String roleRelay1;
extern String roleRelay2;
extern String roleRelay3;
extern String roleRelay4;

// --- 4. EXTERN OBJECTS (LOGIC AUTOMATION) ---
#define MAX_RULES 10 

struct LogicRule {
    String metric;
    String op;
    float value;
    String action;
    unsigned long duration_ms;
    unsigned long cooldown_ms;
};

extern LogicRule rules[MAX_RULES];
extern int ruleCount;

struct ActionState {
    bool isActive;
    unsigned long startTime;
    unsigned long duration;
    unsigned long lastOffTime;
};

extern ActionState relayStates[4]; 

// --- 5. TOPIC MQTT ---
extern String TOPIC_SUB_PREFIX;
extern String TOPIC_SUB_RELAYS;
extern String TOPIC_SUB_MESSAGE;
extern String TOPIC_PUB_STATUS;
extern String TOPIC_PUB_TELEMETRY;
extern String TOPIC_SUB_CONFIG;
extern String TOPIC_PUB_LOG;

#endif
