#ifndef AM2303_H
#define AM2303_H

#define AM2320_ADDR 0x5C

void initSensor();
bool measure();
float getTemperature();
float getHumidity();

#endif