#ifndef NVS_H
#define NVS_H

int getCredentialsFromNVS(char **ssid, char **password);

int saveCredentialsInNVS(const char *ssid, const char *password);

int getCounterFromNVS();

int saveRecordsInNVS(int humidity, int temperature, int insolation);

int getRecordsFromNVS(int *humidity, int *temperature, int *insolation, int data_count);

int clearCounter();

int saveInNVS(const char* key, int value);

int clearCredentialsFromNVS();

int getConfigFromNVSBy(const char *key);
#endif