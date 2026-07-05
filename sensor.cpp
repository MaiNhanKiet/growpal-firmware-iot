#include "sensor.h"
#include "config.h"

float lastTemp = 28.0;
float lastHumid = 65.0;
int lastSoil = 70;

void initSensors() { randomSeed(analogRead(0)); }

SensorData readSensors() {
    SensorData data = {0, 0, 0, 0};

    if (sysState.hasTemp) {
        lastTemp += random(-2, 3) / 10.0;
        if (lastTemp > 35) lastTemp = 35; if (lastTemp < 20) lastTemp = 20;
        data.temp = lastTemp;
    }
    if (sysState.hasHumid) {
        lastHumid += random(-5, 6) / 10.0;
        if (lastHumid > 90) lastHumid = 90;
        data.humid = lastHumid;
    }
    if (sysState.hasSoil) {
        lastSoil += random(-1, 2);
        if (lastSoil > 100) lastSoil = 100; if (lastSoil < 0) lastSoil = 0;
        data.soil = lastSoil;
    }
    if (sysState.hasLight) data.light = random(2000, 4000);

    return data;
}
