#ifndef SENSOR_H
#define SENSOR_H

struct SensorData {
    float temp; float humid; int soil; int light;
};

void initSensors();
SensorData readSensors(); 

#endif
