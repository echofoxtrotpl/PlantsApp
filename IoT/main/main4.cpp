#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <nvs_flash.h>
#include "esp_sleep.h"
#include <driver/gpio.h>

#define TAG "Sensor"
bool button_state = 0;
#define BUTTON_GPIO GPIO_NUM_0



extern "C" void app_main(void)
{
    gpio_install_isr_service(0);

    // Set the button as an input pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1 << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);

    // Create the button detection task
    xTaskCreate(button_detection_task, "button_detection_task", 2048, NULL, 5, NULL);
}