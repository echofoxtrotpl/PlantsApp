#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "cJSON.h"
#include "sdkconfig.h"
#include "ble.h"
#include "nvs.h"

uint8_t ble_addr_type;
static EventGroupHandle_t ble_event_group;
const int BLE_PROVISIONED_BIT = BIT1;

void ble_app_advertise(void);

static int handle_write_config(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char incomingJson[1024];
    memcpy(incomingJson, ctxt->om->om_data, ctxt->om->om_len);
    cJSON *json = cJSON_Parse(incomingJson);

    if (cJSON_GetObjectItem(json, "ssid") && cJSON_GetObjectItem(json, "password"))
    {
        const char *ssid = (const char *)cJSON_GetObjectItem(json, "ssid")->valuestring; 
        const char *password = (const char *)cJSON_GetObjectItem(json, "password")->valuestring;

        ESP_LOGI("BLE", "Received Wi-Fi credentials"
                        "\n\tSSID     : %s\n\tPassword : %s",
                ssid, password);
        saveCredentialsInNVS(ssid, password);
        xEventGroupSetBits(ble_event_group, BLE_PROVISIONED_BIT);
    }

    return 0;
}

// Read data from ESP32 defined as server
// static int handle_read_config(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
// {
//     os_mbuf_append(ctxt->om, "Response from server 123", strlen("Response from server 123"));
//     return 0;
// }

// Array of pointers to other service definitions
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        /*** Service: Security test. */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(0x180),
        .characteristics = (struct ble_gatt_chr_def[]){{
                                                           /*** Characteristic: Random number generator. */
                                                           .uuid = BLE_UUID16_DECLARE(0xDEA0),
                                                           .access_cb = handle_write_config,
                                                           .flags = BLE_GATT_CHR_F_WRITE,
                                                       },
                                                       {
                                                           0, /* No more characteristics in this service. */
                                                       }},
},

                                     {
                                         0, /* No more services. */
                                     },
};

    // BLE event handling
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    // Advertise if connected
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT CONNECT %s", event->connect.status == 0 ? "OK!" : "FAILED!");
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        break;
    // Advertise again after completion of the event
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI("GAP", "BLE GAP EVENT");
        ble_app_advertise();
        break;
    default:
        break;
    }
    return 0;
}

// Define the BLE connection
void ble_app_advertise(void)
{
    // GAP - device name definition
    struct ble_hs_adv_fields fields;
    const char *device_name;
    memset(&fields, 0, sizeof(fields));
    device_name = ble_svc_gap_device_name(); // Read the BLE device name
    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;
    ble_gap_adv_set_fields(&fields);

    // GAP - device connectivity definition
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // connectable or non-connectable
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // discoverable or non-discoverable
    ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
}

// The application
void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type); // Determines the best address type automatically
    ble_app_advertise();                     // Define the BLE connection
}

// The infinite task
void host_task(void *param)
{
    nimble_port_run(); // This function will return only when nimble_port_stop() is executed
}

void init_ble(char* device_name)
{
    ble_event_group = xEventGroupCreate();
    esp_nimble_hci_and_controller_init();  // 2 - Initialize ESP controller
    nimble_port_init();                        // 3 - Initialize the host stack
    ble_svc_gap_device_name_set(device_name);  // 4 - Initialize NimBLE configuration - server name
    ble_svc_gap_init();                        // 4 - Initialize NimBLE configuration - gap service
    ble_svc_gatt_init();                       // 4 - Initialize NimBLE configuration - gatt service
    ble_gatts_count_cfg(gatt_svcs);            // 4 - Initialize NimBLE configuration - config gatt services
    ble_gatts_add_svcs(gatt_svcs);             // 4 - Initialize NimBLE configuration - queues gatt services.
    ble_hs_cfg.sync_cb = ble_app_on_sync;      // 5 - Initialize application
    nimble_port_freertos_init(host_task);      // 6 - Run the thread

    ESP_LOGI("GAP", "Waiting for provision");

    xEventGroupWaitBits(ble_event_group,
                        BLE_PROVISIONED_BIT,
                        false,
                        true,
                        portMAX_DELAY);

    ESP_LOGI("GAP", "Provisioned");

    nimble_port_stop();
    nimble_port_deinit();
    esp_nimble_hci_and_controller_deinit();
}