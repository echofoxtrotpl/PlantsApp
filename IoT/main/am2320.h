#ifndef AM2320_H
#define AM2320_H

#define AM2320_ADDR 0x5C

void initAM2320Sensor();
bool measureTemperatureAndHumidity();
float getTemperature();
float getHumidity();

#endif