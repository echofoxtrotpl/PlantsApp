#include "am2320.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define TAG "Sensor"

extern "C" void app_main(void)
{
    initSensor();

    while (1)
    {
        if (measure())
        {
            ESP_LOGI(TAG, "Temperature: %.2f", getTemperature());
            ESP_LOGI(TAG, "Humidity: %.2f", getHumidity());
        }
        else
        {
            ESP_LOGE(TAG, "Reading error");
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}