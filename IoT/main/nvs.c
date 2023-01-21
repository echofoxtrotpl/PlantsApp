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

    return 1;
}