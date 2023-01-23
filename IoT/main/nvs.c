#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

int saveCredentialsInNVS(const char *ssid, const char *password)
{
    nvs_handle_t my_handle;

    if (nvs_open("storage", NVS_READWRITE, &my_handle) != ESP_OK)
    {
        ESP_LOGE("NVS", "Error opening NVS handle!");
        nvs_close(my_handle);
        return -1;
    }
    else
    {
        if (nvs_set_str(my_handle, "ssid", ssid) != ESP_OK)
        {
            ESP_LOGE("NVS", "Error saving ssid in NVS!");
            nvs_close(my_handle);
            return -1;
        }
        if (nvs_set_str(my_handle, "password", password) != ESP_OK)
        {
            ESP_LOGE("NVS", "Error saving password in NVS!");
            nvs_close(my_handle);
            return -1;
        }
        if (nvs_commit(my_handle) != ESP_OK)
        {
            ESP_LOGE("NVS", "Error saving credentials in NVS!");
            nvs_close(my_handle);
            return -1;
        }

        // Close
        nvs_close(my_handle);
        return 0;
    }
}

int getCredentialsFromNVS(char **ssid, char **password)
{
    nvs_handle_t my_handle;
    size_t required_size;

    if (nvs_open("storage", NVS_READWRITE, &my_handle) != ESP_OK)
    {
        ESP_LOGE("NVS", "Error opening NVS handle!");
        nvs_close(my_handle);
        // -1 means error
        return -1;
    }

    // reading ssid
    if (nvs_get_str(my_handle, "ssid", NULL, &required_size) != ESP_OK)
    {
        ESP_LOGI("NVS", "No ssid saved in NVS");
        nvs_close(my_handle);
        // 0 means no value in nvs
        return 0;
    }
    *ssid = (char *)malloc(required_size);
    nvs_get_str(my_handle, "ssid", *ssid, &required_size);

    // reading password
    if (nvs_get_str(my_handle, "password", NULL, &required_size) != ESP_OK)
    {
        ESP_LOGE("NVS", "No password saved in NVS!");
        nvs_close(my_handle);
        return 0;
    }
    *password = (char *)malloc(required_size);
    nvs_get_str(my_handle, "password", *password, &required_size);
    nvs_close(my_handle);
    return 1;
}

int saveRecordsInNVS(int humidity, int temperature)
{
    nvs_handle_t my_handle;

    if (nvs_open("storage", NVS_READWRITE, &my_handle) != ESP_OK)
    {
        ESP_LOGE("NVS", "Error opening NVS handle!");
        nvs_close(my_handle);
        return -1;
    }
    else
    {

        size_t data_count = getCounterFromNVS();
        ++data_count;

        // set humidity
        char key_humidity[6];
        sprintf(key_humidity, "dh_%d", data_count);

        if (nvs_set_i16(my_handle, key_humidity, humidity) != ESP_OK)
        {
            ESP_LOGE("NVS", "Error saving humidity in NVS!");
            nvs_close(my_handle);
            return -1;
        }

        // set temperature
        char key_temperature[6];
        sprintf(key_temperature, "dt_%d", data_count);

        if (nvs_set_i16(my_handle, key_temperature, temperature) != ESP_OK)
        {
            ESP_LOGE("NVS", "Error saving temperature in NVS!");
            nvs_close(my_handle);
            return -1;
        }

        // update counter
        if (nvs_set_u32(my_handle, "data_count", data_count) != ESP_OK)
        {
            ESP_LOGE("NVS", "Error saving counter in NVS!");
            nvs_close(my_handle);
            return -1;
        }

        if (nvs_commit(my_handle) != ESP_OK)
        {
            ESP_LOGE("NVS", "Error saving data in NVS!");
            nvs_close(my_handle);
            return -1;
        }

        // Close
        nvs_close(my_handle);
        return 0;
    }
}

int getRecordsFromNVS(int *humidity, int *temperature, int data_count)
{
    nvs_handle_t my_handle;
    size_t required_size;

    if (nvs_open("storage", NVS_READWRITE, &my_handle) != ESP_OK)
    {
        ESP_LOGE("NVS", "Error opening NVS handle!");
        nvs_close(my_handle);
        // -1 means error
        return -1;
    }

    // reading temperature
    char key_temperature[6];
    sprintf(key_temperature, "dt_%d", data_count);

    if (nvs_get_i16(my_handle, key_temperature, temperature) != ESP_OK)
    {
        ESP_LOGI("NVS", "No %s saved in NVS", key_temperature);
        nvs_close(my_handle);
        // 0 means no value in nvs
        return 0;
    }

    // reading humidity
    char key_humidity[6];
    sprintf(key_humidity, "dh_%d", data_count);

    if (nvs_get_i16(my_handle, key_humidity, humidity) != ESP_OK)
    {
        ESP_LOGE("NVS", "No %s saved in NVS!", key_humidity);
        nvs_close(my_handle);
        return 0;
    }

    // ESP_LOGI("NVS", "HUM in getRecordsFromNVS: %f", (float)(* humidity) / 100);
    // ESP_LOGI("NVS", "TEMP in getRecordsFromNVS: %f", (float)(* temperature) / 100);
    
    nvs_close(my_handle);
    
    return 1;
}

int getCounterFromNVS()
{
    nvs_handle_t my_handle;
    uint32_t data_count;

    if (nvs_open("storage", NVS_READWRITE, &my_handle) != ESP_OK)
    {
        ESP_LOGE("NVS", "Error opening NVS handle!");
        nvs_close(my_handle);
        return 0;
    }

    if (nvs_get_u32(my_handle, "data_count", &data_count) != ESP_OK)
    {
        ESP_LOGI("NVS", "No counter saved in NVS");
        nvs_close(my_handle);
        // 0 means no value in nvs
        return 0;
    }
    nvs_close(my_handle);
    ESP_LOGI("NVS", "Counter val %d", data_count);
    return data_count;
}

int clearCounter()
{
    nvs_handle_t my_handle;

    if (nvs_open("storage", NVS_READWRITE, &my_handle) != ESP_OK)
    {
        ESP_LOGE("NVS", "Error opening NVS handle!");
        nvs_close(my_handle);
        return -1;
    }
    else
    {
        // update counter
        if (nvs_set_u32(my_handle, "data_count", 0) != ESP_OK)
        {
            ESP_LOGE("NVS", "Error saving counter in NVS!");
            nvs_close(my_handle);
            return -1;
        }

        if (nvs_commit(my_handle) != ESP_OK)
        {
            ESP_LOGE("NVS", "Error saving data in NVS!");
            nvs_close(my_handle);
            return -1;
        }

        // Close
        nvs_close(my_handle);
        return 0;
    }
}

int saveInNVS(const char* key, int value)
{
    nvs_handle_t my_handle;

    if (nvs_open("storage", NVS_READWRITE, &my_handle) != ESP_OK)
    {
        ESP_LOGE("NVS", "Error opening NVS handle!");
        nvs_close(my_handle);
        return -1;
    }
    else
    {
        if (nvs_set_i16(my_handle, key, value) != ESP_OK)
        {
            ESP_LOGE("NVS", "Error %s: %d in NVS!", key, value);
            nvs_close(my_handle);
            return -1;
        }

        if (nvs_commit(my_handle) != ESP_OK)
        {
            ESP_LOGE("NVS", "Error saving data in NVS!");
            nvs_close(my_handle);
            return -1;
        }

        // Close
        nvs_close(my_handle);
        return 0;
    }
}

int getConfigFromNVSBy(const char *key)
{
    nvs_handle_t my_handle;
    uint32_t value;

    if (nvs_open("storage", NVS_READWRITE, &my_handle) != ESP_OK)
    {
        ESP_LOGE("NVS", "Error opening NVS handle!");
        nvs_close(my_handle);
        return -1;
    }

    if (nvs_get_u32(my_handle, key, &value) != ESP_OK)
    {
        ESP_LOGI("NVS", "No %s saved in NVS", key);
        nvs_close(my_handle);
        return -1;
    }
    nvs_close(my_handle);
    return value;
}