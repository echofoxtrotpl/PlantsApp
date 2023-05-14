#include "am2320.h"

#include <Wire.h>

uint8_t _buf[8];
float _temperature;
float _humidity;
int _insolation;

void initSensor()
{
    Wire.begin();
}

float getTemperature()
{
    return _temperature;
}

float getHumidity()
{
    return _humidity;
}

float getInsolation()
{
    return _insolation;
}

void measureTemperatureAndHumidity()
{
    // wybudzenie sensora
    Wire.beginTransmission(AM2320_ADDR);
    Wire.endTransmission();

    // przygotowanie sekwencji 3 bitow oznaczajaca chec czytania
    Wire.beginTransmission(AM2320_ADDR);
    // https://cdn-shop.adafruit.com/product-files/3721/AM2320.pdf str 20
    Wire.write(0x03); // bajt mowiacy ze chcemy czytac
    Wire.write(0x00); // adres poczatkowy
    Wire.write(0x04); // liczba bajtow = 4 wraz z inicjujacym transmisje

    // wyslanie powyzszego zapytania z sprawdzenie statusu
    if (Wire.endTransmission(true) != 0)
    {
        return false; // sensor nie jest gotowy do odczytu
    }

    // https://cdn-shop.adafruit.com/product-files/3721/AM2320.pdf str 17
    delayMicroseconds(1500);          // delay z dokumentacji
    Wire.requestFrom(AM2320_ADDR, 8); // 8 bajtow = 4 bajty danych i 4 bajty na metadata

    // wczytanie kolejno 8 bajtow
    for (int i = 0; i < 8; i++)
    {
        _buf[i] = Wire.read();
    }

    int humudity = ((_buf[2] << 8) | _buf[3]);
    _humidity = humudity / 10.0;

    int temperature = ((_buf[4] & 0x7F) << 8) | _buf[5];
    if ((_buf[4] & 0x80) >> 7 == 1) // gdy temperatura jest ujemna
    {
        _temperature = (temperature / 10.0) * -1;
    }
    else
    {
        _temperature = temperature / 10.0;
    }
}

void measureInsolation() {
    // TODO: add sensor measurement
    _insolation = 54321;
}

bool measure()
{
    measureTemperatureAndHumidity()
    measureInsolation()

    return true;
}