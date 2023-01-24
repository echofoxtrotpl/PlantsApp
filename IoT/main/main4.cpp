#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <nvs_flash.h>
#include "esp_sleep.h"

#define TAG "Sensor"

static const uint64_t TIME_IN_US = 20e6;

void setup_sleep()
{
    esp_sleep_enable_timer_wakeup(TIME_IN_US);
}

extern "C" void app_main(void)
{
    setup_sleep();
    ESP_LOGI("TEST", "SIEMA");
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    esp_deep_sleep_start();

}