#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include "config.h"

void initMqtt();
void loopMqtt();
void publishSensorData(float t, float h, int s, int l);
void publishActionLog(int relayNum, const char* role, const char* action,
                      const char* reason, float value);

/** Process one dequeued MQTT message on Core 1 (JSON / Preferences / UI). */
void processMqttMessage(const MqttMessage* msg);

#endif
