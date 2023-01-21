#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <nvs_flash.h>

extern "C" {
    #include "ble.h"
}

#define TAG "Sensor"

extern "C" void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    char *ssidFromNVS = NULL;
    char *passwordFromNVS = NULL;

    saveCredentialsInNVS("test_ssid", "test_pass");

    if (getCredentialsFromNVS(&ssidFromNVS, &passwordFromNVS) == 1)
    {
        ESP_LOGI(TAG, "Credentials: ssid:%s, pass:%s", ssidFromNVS, passwordFromNVS);
    }
}