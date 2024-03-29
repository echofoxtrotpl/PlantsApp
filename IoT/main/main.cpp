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
#include "Wire.h"

#include "am2320.h"
#include "bh1750.h"
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
    ESP_LOGI(TAG, "Looking for a new firmware...\n");

    esp_http_client_config_t config = {
        .url = UPDATE_JSON_URL,
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
            ESP_LOGE(TAG, "downloaded file is not a valid json, aborting...\n");
        else
        {
            cJSON *version = cJSON_GetObjectItemCaseSensitive(json, "version");
            cJSON *file = cJSON_GetObjectItemCaseSensitive(json, "file");

            // check the version
            if (!cJSON_IsNumber(version))
                ESP_LOGE(TAG, "unable to read new version, aborting...\n");
            else
            {

                double new_version = version->valuedouble;
                if (new_version > FIRMWARE_VERSION)
                {

                    ESP_LOGI(TAG, "current firmware version (%.1f) is lower than the available one (%.1f), upgrading...\n", FIRMWARE_VERSION, new_version);
                    if (cJSON_IsString(file) && (file->valuestring != NULL))
                    {
                        ESP_LOGI(TAG, "downloading and installing new firmware (%s)...\n", file->valuestring);

                        esp_http_client_config_t ota_client_config = {
                            .url = file->valuestring};
                        esp_err_t ret = esp_https_ota(&ota_client_config);
                        if (ret == ESP_OK)
                        {
                            ESP_LOGI(TAG, "OTA OK, restarting...\n");
                            esp_restart();
                        }
                        else
                        {
                            ESP_LOGE(TAG, "OTA failed...\n");
                        }
                    }
                    else
                        ESP_LOGE(TAG, "unable to read the new file name, aborting...\n");
                }
                else
                    ESP_LOGI(TAG, "current firmware version (%.1f) is greater or equal to the available one (%.1f), nothing to do...\n", FIRMWARE_VERSION, new_version);
            }
        }
    }
    else
        ESP_LOGE(TAG, "unable to download the json file, aborting...\n");

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
            if (counter == 3)
            {
                clearCredentialsFromNVS();
                vTaskDelete(NULL);
                esp_restart();
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void configure_reset_credentials_button()
{
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
    topic = topic + "/measurements";
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
    std::string device_name = service_name;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        esp_mqtt_client_subscribe(mqtt_client, (device_name + "/pushInterval").c_str(), 1);
        esp_mqtt_client_subscribe(mqtt_client, (device_name + "/minTemperature").c_str(), 1);
        esp_mqtt_client_subscribe(mqtt_client, (device_name + "/maxTemperature").c_str(), 1);
        esp_mqtt_client_subscribe(mqtt_client, (device_name + "/maxHumidity").c_str(), 1);
        esp_mqtt_client_subscribe(mqtt_client, (device_name + "/minHumidity").c_str(), 1);
        esp_mqtt_client_subscribe(mqtt_client, (device_name + "/maxInsolation").c_str(), 1);
        esp_mqtt_client_subscribe(mqtt_client, (device_name + "/minInsolation").c_str(), 1);

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

        if (strcmp(event->topic, (device_name + "/pushInterval").c_str()) == 0)
        {
            int pushInterval = atoi(event->data);

            saveInNVS("pushInterval", pushInterval);
        }

        if (strcmp(event->topic, (device_name + "/minTemperature").c_str()) == 0)
        {
            float minTemperature = std::stof(event->data);

            saveInNVS("minTemperature", (int)(minTemperature * 100));
        }

        if (strcmp(event->topic, (device_name + "/maxTemperature").c_str()) == 0)
        {
            float maxTemperature = std::stof(event->data);

            saveInNVS("maxTemperature", (int)(maxTemperature * 100));
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

        if (strcmp(event->topic, (device_name + "/minInsolation").c_str()) == 0)
        {
            int minInsolation = atoi(event->data);

            saveInNVS("minInsolation", minInsolation);
        }

        if (strcmp(event->topic, (device_name + "/maxInsolation").c_str()) == 0)
        {
            int maxInsolation = atoi(event->data);

            saveInNVS("maxInsolation", maxInsolation);
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

static char *parseRecord(float hum, float temp, float ins)
{
    cJSON *json = cJSON_CreateObject();

    cJSON *humidity = cJSON_CreateNumber(hum);
    cJSON_AddItemToObject(json, "humidity", humidity);

    cJSON *temperature = cJSON_CreateNumber(temp);
    cJSON_AddItemToObject(json, "temperature", temperature);

    cJSON *insolation = cJSON_CreateNumber(ins);
    cJSON_AddItemToObject(json, "insolation", insolation);

    return cJSON_Print(json);
}

char *create_json(float currTemperature, float currHumidity, float currInsolation)
{
    int temperature = 0;
    int humidity = 0;
    int insolation = 0;

    cJSON *root, *measurements;

    root = cJSON_CreateObject();
    measurements = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "measurements", measurements);

    // add current record
    cJSON *measurement = cJSON_CreateObject();

    cJSON_AddItemToObject(measurement, "temperature", cJSON_CreateNumber(currTemperature));
    cJSON_AddItemToObject(measurement, "humidity", cJSON_CreateNumber(currHumidity));
    cJSON_AddItemToObject(measurement, "insolation", cJSON_CreateNumber(currInsolation));

    cJSON_AddItemToArray(measurements, measurement);

    // send all collected temperatures from NVS to MQTT
    int counter = getCounterFromNVS();
    int interval = getConfigFromNVSBy("");
    for (int i = counter; i >= 1; --i)
    {
        getRecordsFromNVS(&humidity, &temperature, &insolation, i);

        cJSON *measurement = cJSON_CreateObject();

        cJSON_AddItemToObject(measurement, "temperature", cJSON_CreateNumber((float)temperature / 100));
        cJSON_AddItemToObject(measurement, "humidity", cJSON_CreateNumber((float)humidity / 100));
        cJSON_AddItemToObject(measurement, "insolation", cJSON_CreateNumber((float)insolation / 100));

        cJSON_AddItemToArray(measurements, measurement);
    }

    // add interval
    cJSON_AddItemToObject(root, "interval", cJSON_CreateNumber());

    char *out = cJSON_Print(root);
    ESP_LOGI(TAG, "%s", out);
    // cJSON_Delete(measurements);
    cJSON_Delete(root);

    return out;
}

void push_data_to_server(float currTemperature, float currHumidity, float currInsolation)
{
    char *json = create_json(currTemperature, currHumidity, currInsolation);
    // TODO: add real path and decide about name
    char URL[150] = "http://127.0.0.1:8880/weatherstation/api/stations/";
    strcat(URL, "7f0275e4-56f1-4d79-8ad7-f159675818c5");
    strcat(URL, "/measurements");
    ESP_LOGI(TAG, "URL: %s", URL);
    esp_http_client_config_t config = {
        .url = URL,
        .method = HTTP_METHOD_POST,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_post_field(client, json, strlen(json));
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG, "Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(TAG, "Error perform http request %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    free(json);
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

bool shouldSendData(float currHumidity, float currTemperature, float currInsolation)
{
    int pushIntervalFromNVS = getConfigFromNVSBy("pushInterval");
    int pushInterval = pushIntervalFromNVS == -1 ? 10 : pushIntervalFromNVS;

    int minTempFromNVS = getConfigFromNVSBy("minTemperature");
    float minTemperature = minTempFromNVS == -1 ? 16 : (float)minTempFromNVS / 100;

    int maxTempFromNVS = getConfigFromNVSBy("maxTemperature");
    float maxTemperature = maxTempFromNVS == -1 ? 27 : (float)maxTempFromNVS / 100;

    int minHumidity = getConfigFromNVSBy("minHumidity");
    minHumidity = minHumidity == -1 ? 20 : minHumidity;

    int maxHumidity = getConfigFromNVSBy("maxHumidity");
    maxHumidity = maxHumidity == -1 ? 80 : maxHumidity;

    int minInsolation = getConfigFromNVSBy("minHumidity");
    minInsolation = minInsolation == -1 ? 1 : minInsolation;

    int maxInsolation = getConfigFromNVSBy("maxInsolation");
    maxInsolation = maxInsolation == -1 ? 65350 : maxInsolation;

    int counter = getCounterFromNVS();
    ESP_LOGI(TAG, "Counter in main: %d", counter);

    return (counter != 0 && counter % pushInterval == 0) || currTemperature <= minTemperature || currTemperature >= maxTemperature || currHumidity <= minHumidity || currHumidity >= maxHumidity || currInsolation <= minInsolation || currInsolation >= maxInsolation;
}

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

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    configure_led();
    configure_reset_credentials_button();
    turn_on_led_for(5000);

    setup_sleep();
    initAM2320Sensor();
    initBH1750Sensor();
    get_device_service_name(service_name, sizeof(service_name));

    char *ssidFromNVS = NULL;
    char *passwordFromNVS = NULL;

    if (getCredentialsFromNVS(&ssidFromNVS, &passwordFromNVS) == 0)
    {
        // saveCredentialsInNVS("NaszaSiec.NET_43D4A9", "5OhTC9RSHS");
        provision_device(service_name);
    }

    while (1)
    {
        if (measureTemperatureAndHumidity() && measureInsolation())
        {
            float currTemperature = getTemperature();
            float currHumidity = getHumidity();
            float currInsolation = getInsolation();

            ESP_LOGI(TAG, "Current humidity: %f", currHumidity);
            ESP_LOGI(TAG, "Current temperature: %f", currTemperature);
            ESP_LOGI(TAG, "Current insolation: %f", currInsolation);

            if (shouldSendData(currHumidity, currTemperature, currInsolation))
            {
                if (getCredentialsFromNVS(&ssidFromNVS, &passwordFromNVS) == 1)
                {
                    ESP_LOGI(TAG, "Already provisioned");

                    if (start_wifi(ssidFromNVS, passwordFromNVS) != 1)
                    {
                        saveRecordsInNVS((uint32_t)(currHumidity * 100), (uint32_t)(currTemperature * 100), (uint32_t)(currInsolation * 100));
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
                        saveRecordsInNVS((uint32_t)(currHumidity * 100), (uint32_t)(currTemperature * 100), (uint32_t)(currInsolation * 100));
                        ESP_LOGE(TAG, "Couldn't connect, restarting");
                        esp_restart();
                    }
                }

                // check_update();
                mqtt_event_group = xEventGroupCreate();

                mqtt_app_start();

                xEventGroupWaitBits(mqtt_event_group,
                                    MQTT_CONNECTED_BIT,
                                    false,
                                    false,
                                    portMAX_DELAY);

                push_data_to_server(currTemperature, currHumidity, currInsolation);

                // send current temperature to MQTT
                esp_mqtt_client_publish(mqtt_client, topic.c_str(), parseRecord(currHumidity, currTemperature, currInsolation), 0, 1, 0);
                xEventGroupWaitBits(mqtt_event_group,
                                    MQTT_PUBLISHED_BIT,
                                    false,
                                    false,
                                    portMAX_DELAY);
                xEventGroupClearBits(mqtt_event_group, MQTT_PUBLISHED_BIT);

                clearCounter();

                esp_event_loop_delete_default();
                esp_mqtt_client_stop(mqtt_client);
                esp_wifi_stop();
            }
            else
            {
                saveRecordsInNVS((uint32_t)(currHumidity * 100), (uint32_t)(currTemperature * 100), (uint32_t)(currInsolation * 100));
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
