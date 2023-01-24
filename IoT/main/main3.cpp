#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cJSON.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>

#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_ble.h>

#include "driver/gpio.h"

#include <WiFiUdp.h>
#include "mqtt_client.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_sleep.h"
#include <time.h>
#include <driver/gpio.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "am2320.h"
extern "C"
{
#include "nvs.h"
#include "ble.h"
#include "wifi.h"
}

#define FIRMWARE_VERSION 0.2
#define UPDATE_JSON_URL "http://192.168.20.102:8000/ota.json"
#define BLINK_GPIO GPIO_NUM_2
#define BUTTON_GPIO GPIO_NUM_0

static const char *TAG = "app";
static const char *CONFIG_BROKER_URL = "mqtt://mqtt.eclipseprojects.io";

static const uint64_t TIME_IN_US = 20e6; // 20 sekund

// receive buffer
char rcv_buffer[100];

bool button_state = 0;

// esp_http_client event handler
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{

    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGI(TAG, "error in connection");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "Connected");
        break;
    case HTTP_EVENT_HEADER_SENT:
        break;
    case HTTP_EVENT_ON_HEADER:
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "Got data");
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            strncpy(rcv_buffer, (char *)evt->data, evt->data_len);
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        break;
    case HTTP_EVENT_DISCONNECTED:
        break;
    }
    return ESP_OK;
}

// Check update task
void check_update()
{
    printf("Looking for a new firmware...\n");

    // configure the esp_http_client
    esp_http_client_config_t config = {
        .url = UPDATE_JSON_URL,
        //.cert_pem = (char *)server_cert_pem_start,
        .timeout_ms = 10000,
        .event_handler = _http_event_handler,

    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // downloading the json file
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {

        // parse the json file
        cJSON *json = cJSON_Parse(rcv_buffer);
        if (json == NULL)
            printf("downloaded file is not a valid json, aborting...\n");
        else
        {
            cJSON *version = cJSON_GetObjectItemCaseSensitive(json, "version");
            cJSON *file = cJSON_GetObjectItemCaseSensitive(json, "file");

            // check the version
            if (!cJSON_IsNumber(version))
                printf("unable to read new version, aborting...\n");
            else
            {

                double new_version = version->valuedouble;
                if (new_version > FIRMWARE_VERSION)
                {

                    printf("current firmware version (%.1f) is lower than the available one (%.1f), upgrading...\n", FIRMWARE_VERSION, new_version);
                    if (cJSON_IsString(file) && (file->valuestring != NULL))
                    {
                        printf("downloading and installing new firmware (%s)...\n", file->valuestring);

                        esp_http_client_config_t ota_client_config = {
                            .url = file->valuestring,
                        };
                        esp_err_t ret = esp_https_ota(&ota_client_config);
                        if (ret == ESP_OK)
                        {
                            printf("OTA OK, restarting...\n");
                            esp_restart();
                        }
                        else
                        {
                            printf("OTA failed...\n");
                        }
                    }
                    else
                        printf("unable to read the new file name, aborting...\n");
                }
                else
                    printf("current firmware version (%.1f) is greater or equal to the available one (%.1f), nothing to do...\n", FIRMWARE_VERSION, new_version);
            }
        }
    }
    else
        printf("unable to download the json file, aborting...\n");

    // cleanup
    esp_http_client_cleanup(client);
}

/* Signal Wi-Fi events on this event-group */
const int MQTT_PUBLISHED_BIT = BIT3;
const int MQTT_CONNECTED_BIT = BIT1;
static EventGroupHandle_t mqtt_event_group;

esp_mqtt_client_handle_t mqtt_client;

char service_name[12];
std::string topic;

static void turn_on_led_for(int time)
{
    gpio_set_level(BLINK_GPIO, 1);
    vTaskDelay(time / portTICK_PERIOD_MS);
    gpio_set_level(BLINK_GPIO, 0);
}

static void configure_led(void)
{
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

void configure_potentiometer()
{
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
}

void button_detection_task(void *arg)
{
    int counter = 0;
    while (1)
    {
        // Read the current state of the button
        bool new_state = gpio_get_level(BUTTON_GPIO);

        if (new_state != button_state)
        {

            if (new_state == 0)
            {
                ++counter;
            }

            button_state = new_state;
            // reset credentials after 3 clicks
            if(counter == 3) {
                clearCredentialsFromNVS();
                vTaskDelete(NULL);
                esp_restart();
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void configure_reset_credentials_button() {
    gpio_install_isr_service(0);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1 << BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&io_conf);

    xTaskCreate(button_detection_task, "button_detection_task", 2048, NULL, 5, NULL);
}

void setup_sleep()
{
    esp_sleep_enable_timer_wakeup(TIME_IN_US);
}

static void get_device_service_name(char *service_name, size_t max)
{
    init_wifi();
    uint8_t eth_mac[6];
    const char *ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X",
             ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
    topic = service_name;
    topic = topic + "/records";
}

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    mqtt_client = event->client;
    int msg_id;
    ESP_LOGI(TAG, "Service name in mqtt handler: %s", service_name);
    std::string device_name = service_name;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        esp_mqtt_client_subscribe(mqtt_client, (device_name + "/numOfReads").c_str(), 1);
        ESP_LOGI(TAG, "SUBSCRIBED on topic: %s", (device_name + "/numOfReads").c_str());
        esp_mqtt_client_subscribe(mqtt_client, (device_name + "/minTemperature").c_str(), 1);
        esp_mqtt_client_subscribe(mqtt_client, (device_name + "/maxTemperature").c_str(), 1);
        esp_mqtt_client_subscribe(mqtt_client, (device_name + "/maxHumidity").c_str(), 1);
        esp_mqtt_client_subscribe(mqtt_client, (device_name + "/minHumidity").c_str(), 1);

        xEventGroupSetBits(mqtt_event_group, MQTT_CONNECTED_BIT);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        xEventGroupClearBits(mqtt_event_group, MQTT_CONNECTED_BIT);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        xEventGroupSetBits(mqtt_event_group, MQTT_PUBLISHED_BIT);
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s\r\n", event->data_len, event->data);

        if (strcmp(event->topic, (device_name + "/numOfReads").c_str()) == 0)
        {
            int numOfReads = atoi(event->data);

            saveInNVS("numOfReads", numOfReads);
        }

        if (strcmp(event->topic, (device_name + "/minTemperature").c_str()) == 0)
        {
            int minTemperature = atoi(event->data);

            saveInNVS("minTemperature", minTemperature);
        }

        if (strcmp(event->topic, (device_name + "/maxTemperature").c_str()) == 0)
        {
            int maxTemperature = atoi(event->data);

            saveInNVS("maxTemperature", maxTemperature);
        }

        if (strcmp(event->topic, (device_name + "/minHumidity").c_str()) == 0)
        {
            int minHumidity = atoi(event->data);

            saveInNVS("minHumidity", minHumidity);
        }

        if (strcmp(event->topic, (device_name + "/maxHumidity").c_str()) == 0)
        {
            int maxHumidity = atoi(event->data);

            saveInNVS("maxHumidity", maxHumidity);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static char *parseRecord(float hum, float temp)
{
    cJSON *json = cJSON_CreateObject();

    cJSON *humidity = cJSON_CreateNumber(hum);
    cJSON_AddItemToObject(json, "humidity", humidity);

    cJSON *temperature = cJSON_CreateNumber(temp);
    cJSON_AddItemToObject(json, "temperature", temperature);

    cJSON *updatedAt = cJSON_CreateNumber((int)time(NULL));
    cJSON_AddItemToObject(json, "updatedAt", updatedAt);

    return cJSON_Print(json);
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

extern "C" void app_main(void)
{
    /* Initialize NVS partition */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        /* NVS partition was truncated
         * and needs to be erased */
        ESP_ERROR_CHECK(nvs_flash_erase());

        /* Retry nvs_flash_init */
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    configure_led();
    configure_potentiometer();
    int analog_value = adc1_get_raw(ADC1_CHANNEL_0);
    if(analog_value != 0) {
        configure_reset_credentials_button();
        turn_on_led_for(analog_value);
    }
    setup_sleep();
    initSensor();
    get_device_service_name(service_name, sizeof(service_name));

    char *ssidFromNVS = NULL;
    char *passwordFromNVS = NULL;

    if (getCredentialsFromNVS(&ssidFromNVS, &passwordFromNVS) == 0)
    {
        provision_device(service_name);
    }

    while (1)
    {
        if (measure())
        {
            getRecordsFromMiDevice();
                // getTemperatureAndHumidityFromMI();
            float currTemperature = getTemperature();
            float currHumidity = getHumidity();

            ESP_LOGI(TAG, "Current humidity: %f", currHumidity);
            ESP_LOGI(TAG, "Current temperature: %f", currTemperature);

            // 5 ma byc po mqtt
            int numOfReads = getConfigFromNVSBy("numOfReads");
            numOfReads = numOfReads == -1 ? 5 : numOfReads;
            numOfReads *= 60 * 1000 * 1000 / TIME_IN_US;

            int minTemperature = getConfigFromNVSBy("minTemperature");
            minTemperature = minTemperature == -1 ? 16 : minTemperature;

            int maxTemperature = getConfigFromNVSBy("maxTemperature");
            maxTemperature = maxTemperature == -1 ? 27 : maxTemperature;

            int minHumidity = getConfigFromNVSBy("minHumidity");
            minHumidity = minHumidity == -1 ? 20 : minHumidity;

            int maxHumidity = getConfigFromNVSBy("maxHumidity");
            maxHumidity = maxHumidity == -1 ? 80 : maxHumidity;

            int counter = getCounterFromNVS();
            ESP_LOGI(TAG, "Counter in main: %d", counter);
            if ((counter != 0 && counter % numOfReads == 0) || currTemperature <= minTemperature || currTemperature >= maxTemperature || currHumidity <= minHumidity || currHumidity >= maxHumidity)
            {
                if (getCredentialsFromNVS(&ssidFromNVS, &passwordFromNVS) == 1)
                {
                    ESP_LOGI(TAG, "Already provisioned");

                    if (start_wifi(ssidFromNVS, passwordFromNVS) != 1)
                    {
                        saveRecordsInNVS((uint32_t)(currHumidity * 100), (uint32_t)(currTemperature * 100));
                        ESP_LOGE(TAG, "Couldn't connect, restarting");
                        esp_restart();
                    }
                }
                else
                {
                    ESP_LOGI(TAG, "Waiting for provisioning");
                    provision_device(service_name);
                    ESP_LOGI(TAG, "After provisioning");
                    getCredentialsFromNVS(&ssidFromNVS, &passwordFromNVS);
                    if (start_wifi(ssidFromNVS, passwordFromNVS) != 1)
                    {
                        saveRecordsInNVS((uint32_t)(currHumidity * 100), (uint32_t)(currTemperature * 100));
                        ESP_LOGE(TAG, "Couldn't connect, restarting");
                        esp_restart();
                    }
                }

                check_update();
                mqtt_event_group = xEventGroupCreate();

                mqtt_app_start();

                xEventGroupWaitBits(mqtt_event_group,
                                    MQTT_CONNECTED_BIT,
                                    false,
                                    false,
                                    portMAX_DELAY);

                int temperature = 0;
                int humidity = 0;

                // wyślij wszystkie temperatury na mqtt
                for (int i = 1; i <= counter; ++i)
                {
                    getRecordsFromNVS(&humidity, &temperature, i);

                    ESP_LOGI(TAG, "HUM in loop from nvs: %f", (float)humidity / 100);
                    ESP_LOGI(TAG, "TEMP in loop from nvs: %f", (float)temperature / 100);

                    esp_mqtt_client_publish(mqtt_client, topic.c_str(), parseRecord((float)humidity / 100, (float)temperature / 100), 0, 1, 1);
                    xEventGroupWaitBits(mqtt_event_group,
                                        MQTT_PUBLISHED_BIT,
                                        false,
                                        false,
                                        portMAX_DELAY);
                    xEventGroupClearBits(mqtt_event_group, MQTT_PUBLISHED_BIT);
                }

                if (currTemperature <= minTemperature || currTemperature >= maxTemperature || currHumidity <= minHumidity || currHumidity >= maxHumidity)
                {
                    esp_mqtt_client_publish(mqtt_client, topic.c_str(), parseRecord(currHumidity, currTemperature), 0, 1, 1);
                    xEventGroupWaitBits(mqtt_event_group,
                                        MQTT_PUBLISHED_BIT,
                                        false,
                                        false,
                                        portMAX_DELAY);
                    xEventGroupClearBits(mqtt_event_group, MQTT_PUBLISHED_BIT);
                }

                clearCounter();

                esp_event_loop_delete_default();
                esp_mqtt_client_stop(mqtt_client);
                esp_wifi_stop();
            }
            else
            {
                saveRecordsInNVS((uint32_t)(currHumidity * 100), (uint32_t)(currTemperature * 100));
            }

            esp_deep_sleep_start();
        }
        else
        {
            ESP_LOGE(TAG, "Sensor is offline.");
            esp_deep_sleep_start();
        }
    }
}