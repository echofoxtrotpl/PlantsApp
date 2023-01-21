#ifndef NVS_H
#define NVS_H

int getCredentialsFromNVS(char **ssid, char **password);

int saveCredentialsInNVS(const char *ssid, const char *password);

#endif