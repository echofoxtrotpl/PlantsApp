#include <esp_wifi.h>
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "wifi.h"
#include <string.h>

#define WIFI_BIT BIT0
    char *TAG = "WIFI";

static EventGroupHandle_t wifi_event_group;
static int connected = 0;
static int tries = 1;

/* Event handler for catching system events */
static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        connected = 1;
        xEventGroupSetBits(wifi_event_group, WIFI_BIT);
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if(tries == 5){
            connected = -1;
            xEventGroupSetBits(wifi_event_group, WIFI_BIT);
            return;
        }
        tries += 1;
        ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
        //blink_led();
        esp_wifi_connect();
    }
}

void init_wifi() {
    /* Initialize TCP/IP */
    ESP_ERROR_CHECK(esp_netif_init());

    /* Register our event handler for Wi-Fi, IP events */
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    /* Initialize Wi-Fi including netif with default config */
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

int start_wifi(const char* ssid, const char* password) {
    wifi_event_group = xEventGroupCreate();

    wifi_config_t wifi_config = {
        .sta = {
            /* Incase Access point doesn't support WPA2, these mode can be enabled by commenting below line */
            .threshold.authmode = WIFI_AUTH_OPEN,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    strcpy((char *)wifi_config.sta.ssid, ssid);
    strcpy((char *)wifi_config.sta.password, password);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    // wait for 5 tries or connection success
    xEventGroupWaitBits(wifi_event_group,
                        WIFI_BIT,
                        false,
                        true,
                        portMAX_DELAY);

    return connected;
}