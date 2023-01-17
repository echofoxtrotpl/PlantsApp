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

#include "nvs.h"
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

#include "am2320.h"

#define FIRMWARE_VERSION 0.1
#define UPDATE_JSON_URL "https://airsoftmeeting.pl/uploads/ota.json"
#define BLINK_GPIO GPIO_NUM_2

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

static const char *TAG = "app";
static const char *CONFIG_BROKER_URL = "mqtt://mqtt.eclipseprojects.io";

// receive buffer
char rcv_buffer[200];

// esp_http_client event handler
esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    
	switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            break;
        case HTTP_EVENT_ON_CONNECTED:
            break;
        case HTTP_EVENT_HEADER_SENT:
            break;
        case HTTP_EVENT_ON_HEADER:
            break;
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
				strncpy(rcv_buffer, (char*)evt->data, evt->data_len);
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
// downloads every 30sec the json file with the latest firmware
void check_update_task(void *pvParameter) {
	
	while(1) {
        
		printf("Looking for a new firmware...\n");
	
		// configure the esp_http_client
		esp_http_client_config_t config = {
            .url = UPDATE_JSON_URL,
            .event_handler = _http_event_handler,
		};
		esp_http_client_handle_t client = esp_http_client_init(&config);
	
		// downloading the json file
		esp_err_t err = esp_http_client_perform(client);
		if(err == ESP_OK) {
			
			// parse the json file	
			cJSON *json = cJSON_Parse(rcv_buffer);
			if(json == NULL) printf("downloaded file is not a valid json, aborting...\n");
			else {	
				cJSON *version = cJSON_GetObjectItemCaseSensitive(json, "version");
				cJSON *file = cJSON_GetObjectItemCaseSensitive(json, "file");
				
				// check the version
				if(!cJSON_IsNumber(version)) printf("unable to read new version, aborting...\n");
				else {
					
					double new_version = version->valuedouble;
					if(new_version > FIRMWARE_VERSION) {
						
						printf("current firmware version (%.1f) is lower than the available one (%.1f), upgrading...\n", FIRMWARE_VERSION, new_version);
						if(cJSON_IsString(file) && (file->valuestring != NULL)) {
							printf("downloading and installing new firmware (%s)...\n", file->valuestring);
							
							esp_http_client_config_t ota_client_config = {
								.url = file->valuestring,
								.cert_pem = (char *)server_cert_pem_start,
							};
							esp_err_t ret = esp_https_ota(&ota_client_config);
							if (ret == ESP_OK) {
								printf("OTA OK, restarting...\n");
								esp_restart();
							} else {
								printf("OTA failed...\n");
							}
						}
						else printf("unable to read the new file name, aborting...\n");
					}
					else printf("current firmware version (%.1f) is greater or equal to the available one (%.1f), nothing to do...\n", FIRMWARE_VERSION, new_version);
				}
			}
		}
		else printf("unable to download the json file, aborting...\n");
		
		// cleanup
		esp_http_client_cleanup(client);
		
		printf("\n");
        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
}

/* Signal Wi-Fi events on this event-group */
const int WIFI_CONNECTED_BIT = BIT0;
const int MQTT_CONNECTED_BIT = BIT1;
static EventGroupHandle_t wifi_event_group;
esp_mqtt_client_handle_t mqtt_client;

char service_name[12];
float temperature = 0.0;
float humidity = 0.0;
std::string topic;

static void blink_led(void)
{
    gpio_set_level(BLINK_GPIO, 1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    gpio_set_level(BLINK_GPIO, 0);
}

static void configure_led(void)
{
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

void setup_sleep()
{
    // konfiguracja pinu wakeup
    // esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0); // 0 - stan niski, 1 - stan wysoki
    // ustawienie czasu snu
    uint64_t time_in_us = 20e6; // 20 sekund
    printf("time in use: %lld", time_in_us);
    esp_sleep_enable_timer_wakeup(time_in_us);
}

/* Event handler for catching system events */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_PROV_EVENT)
    {
        switch (event_id)
        {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;
        case WIFI_PROV_CRED_RECV:
        {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG, "Received Wi-Fi credentials"
                          "\n\tSSID     : %s\n\tPassword : %s",
                     (const char *)wifi_sta_cfg->ssid,
                     (const char *)wifi_sta_cfg->password);
            break;
        }
        case WIFI_PROV_CRED_FAIL:
        {
            wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                          "\n\tPlease reset to factory and retry provisioning",
                     (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");

            ESP_LOGI(TAG, "Failed to connect with provisioned AP, reseting provisioned credentials");
            wifi_prov_mgr_reset_sm_state_on_failure();
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning successful");
            break;
        case WIFI_PROV_END:
            /* De-initialize manager once provisioning is finished */
            wifi_prov_mgr_deinit();
            break;
        default:
            break;
        }
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        blink_led();
        esp_wifi_connect();
    }
}

static void wifi_init_sta(void)
{
    /* Start Wi-Fi in station mode */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void get_device_service_name(char *service_name, size_t max)
{
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

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    mqtt_client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

        xEventGroupSetBits(wifi_event_group, MQTT_CONNECTED_BIT);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        xEventGroupClearBits(wifi_event_group, MQTT_CONNECTED_BIT);
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
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

    return cJSON_Print(json);
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg;
    mqtt_cfg.uri = CONFIG_BROKER_URL;
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void saveRecordInNVS(float humidity, float temperature)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
        printf("Opened NVS\n");

        err = nvs_set_str(my_handle, "temperature", std::to_string(temperature).c_str());
        printf((err != ESP_OK) ? "Temperature nvs save failed!\n" : "Temperature nvs save done\n");

        err = nvs_set_str(my_handle, "humidity", std::to_string(humidity).c_str());
        printf((err != ESP_OK) ? "Humidity nvs save failed!\n" : "Humidity nvs save done\n");

        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed to save!\n" : "Save done\n");

        // Close
        nvs_close(my_handle);
    }
}

void getRecordFromNVS(float &humidity, float &temperature)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);

    if (err != ESP_OK)
    {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return;
    }

    printf("Opened NVS\n");
    size_t required_size;

    err = nvs_get_str(my_handle, "temperature", NULL, &required_size);
    char *temperatureFromNVS = (char *)malloc(required_size);
    err = nvs_get_str(my_handle, "temperature", temperatureFromNVS, &required_size);

    err = nvs_get_str(my_handle, "humidity", NULL, &required_size);
    char *humidityFromNVS = (char *)malloc(required_size);
    err = nvs_get_str(my_handle, "humidity", humidityFromNVS, &required_size);

    switch (err)
    {
    case ESP_OK:
        humidity = std::stof(humidityFromNVS);
        temperature = std::stof(temperatureFromNVS);
        break;
    case ESP_ERR_NVS_NOT_FOUND:
        nvs_close(my_handle);
        humidity = 0.0f;
        temperature = 0.0f;
        saveRecordInNVS(humidity, temperature);
        break;
    default:
        printf("Error (%s) reading!\n", esp_err_to_name(err));
    }
    free(temperatureFromNVS);
    free(humidityFromNVS);
}

extern "C" void app_main(void)
{
    xTaskCreate(&check_update_task, "check_update_task", 8192, NULL, 5, NULL);
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

    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* conf led */
    configure_led();
    setup_sleep();

    wifi_event_group = xEventGroupCreate();

    /* Register our event handler for Wi-Fi, IP and Provisioning related events */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    /* Initialize Wi-Fi including netif with default config */
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Configuration for the provisioning manager */
    wifi_prov_mgr_config_t config = {
        /* What is the Provisioning Scheme that we want ?
         * wifi_prov_scheme_softap or wifi_prov_scheme_ble */
        .scheme = wifi_prov_scheme_ble,

        /* Any default scheme specific event handler that you would
         * like to choose. Since our example application requires
         * neither BT nor BLE, we can choose to release the associated
         * memory once provisioning is complete, or not needed
         * (in case when device is already provisioned). Choosing
         * appropriate scheme specific event handler allows the manager
         * to take care of this automatically. This can be set to
         * WIFI_PROV_EVENT_HANDLER_NONE when using wifi_prov_scheme_softap*/
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM};

    /* Initialize provisioning manager with the
     * configuration parameters set above */
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

    bool provisioned = false;

    /* Let's find out if the device is provisioned */
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    /* If device is not yet provisioned start provisioning service */
    /* What is the Device Service Name that we want
     * This translates to :
     *     - Wi-Fi SSID when scheme is wifi_prov_scheme_softap
     *     - device name when scheme is wifi_prov_scheme_ble
     */
    get_device_service_name(service_name, sizeof(service_name));
    if (!provisioned)
    {
        ESP_LOGI(TAG, "Starting provisioning");

        /* What is the security level that we want (0 or 1):
         *      - WIFI_PROV_SECURITY_0 is simply plain text communication.
         *      - WIFI_PROV_SECURITY_1 is secure communication which consists of secure handshake
         *          using X25519 key exchange and proof of possession (pop) and AES-CTR
         *          for encryption/decryption of messages.
         */
        wifi_prov_security_t security = WIFI_PROV_SECURITY_1;

        /* Do we want a proof-of-possession (ignored if Security 0 is selected):
         *      - this should be a string with length > 0
         *      - NULL if not used
         */
        const char *pop = "abcd1234";

        /* What is the service key (could be NULL)
         * This translates to :
         *     - Wi-Fi password when scheme is wifi_prov_scheme_softap
         *          (Minimum expected length: 8, maximum 64 for WPA2-PSK)
         *     - simply ignored when scheme is wifi_prov_scheme_ble
         */
        const char *service_key = NULL;

        /* This step is only useful when scheme is wifi_prov_scheme_ble. This will
         * set a custom 128 bit UUID which will be included in the BLE advertisement
         * and will correspond to the primary GATT service that provides provisioning
         * endpoints as GATT characteristics. Each GATT characteristic will be
         * formed using the primary service UUID as base, with different auto assigned
         * 12th and 13th bytes (assume counting starts from 0th byte). The client side
         * applications must identify the endpoints by reading the User Characteristic
         * Description descriptor (0x2901) for each characteristic, which contains the
         * endpoint name of the characteristic */
        uint8_t custom_service_uuid[] = {
            /* LSB <---------------------------------------
             * ---------------------------------------> MSB */
            0xb4,
            0xdf,
            0x5a,
            0x1c,
            0x3f,
            0x6b,
            0xf4,
            0xbf,
            0xea,
            0x4a,
            0x82,
            0x03,
            0x04,
            0x90,
            0x1a,
            0x02,
        };

        /* If your build fails with linker errors at this point, then you may have
         * forgotten to enable the BT stack or BTDM BLE settings in the SDK (e.g. see
         * the sdkconfig.defaults in the example project) */
        wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);

        /* An optional endpoint that applications can create if they expect to
         * get some additional custom data during provisioning workflow.
         * The endpoint name can be anything of your choice.
         * This call must be made before starting the provisioning.
         */
        /* Start provisioning service */
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(security, pop, service_name, service_key));

        /* The handler for the optional endpoint created above.
         * This call must be made after starting the provisioning, and only if the endpoint
         * has already been created above.
         */

        /* Uncomment the following to wait for the provisioning to finish and then release
         * the resources of the manager. Since in this case de-initialization is triggered
         * by the default event loop handler, we don't need to call the following */
        wifi_prov_mgr_wait();
        wifi_prov_mgr_deinit();
    }
    else
    {
        /* We don't need the manager as device is already provisioned,
         * so let's release it's resources */
        ESP_LOGI(TAG, "Already provisioned");
        wifi_prov_mgr_deinit();
    }

    initSensor();

    while (1)
    {
        if (measure())
        {
            float currTemperature = getTemperature();
            float currHumidity = getHumidity();
            // getRecordFromNVS(humidity, temperature);

            ESP_LOGI(TAG, "HUM from nvs: %f", humidity);
            ESP_LOGI(TAG, "TEMP from nvs: %f", temperature);

            if (abs(currHumidity - humidity) >= 0.5 || abs(currTemperature - temperature) >= 0.1)
            {
                // humidity = currHumidity;
                // temperature = currTemperature;
                // ESP_LOGI(TAG, "Temperature: %.2f", temperature);
                // ESP_LOGI(TAG, "Humidity: %.2f", humidity);

                wifi_init_sta();
                xEventGroupWaitBits(wifi_event_group,
                                    WIFI_CONNECTED_BIT,
                                    false,
                                    false,
                                    portMAX_DELAY);

                mqtt_app_start();

                xEventGroupWaitBits(wifi_event_group,
                                    MQTT_CONNECTED_BIT,
                                    false,
                                    false,
                                    portMAX_DELAY);

                esp_mqtt_client_publish(mqtt_client, topic.c_str(), parseRecord(currHumidity, currTemperature), 0, 1, 0);
                // saveRecordInNVS(currHumidity, currTemperature);
            }

            esp_event_loop_delete_default();

            esp_mqtt_client_stop(mqtt_client);
            esp_wifi_stop();
            esp_deep_sleep_start();
        }
        else
        {
            ESP_LOGE(TAG, "Sensor is offline.");
        }
    }
}