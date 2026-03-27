#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>

void initMqtt();
void loopMqtt();
void publishSensorData(float t, float h, int s, int l);
void publishActionLog(int relayNum, String role, String action, String reason, float value);

#endif
