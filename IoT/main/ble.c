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
// #include "esp_bt.h"
// #include "esp_gap_ble_api.h"
// #include "esp_gattc_api.h"
// #include "esp_gatt_defs.h"
// #include "esp_bt_main.h"
// #include "esp_gatt_common_api.h"
#include "sdkconfig.h"
#include "ble.h"
#include "nvs.h"

uint8_t ble_addr_type;
static EventGroupHandle_t ble_event_group;
const int BLE_PROVISIONED_BIT = BIT1;
const int BLE_STOPPED_BIT = BIT5;

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
    nimble_port_deinit();
    esp_nimble_hci_and_controller_deinit();
    xEventGroupSetBits(ble_event_group, BLE_STOPPED_BIT);
    vTaskDelete(NULL);
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
    xEventGroupWaitBits(ble_event_group,
                        BLE_STOPPED_BIT,
                        false,
                        true,
                        portMAX_DELAY);
}

// ble client from here

// struct gattc_profile_inst
// {
//     esp_gattc_cb_t gattc_cb;
//     uint16_t gattc_if;
//     uint16_t app_id;
//     uint16_t conn_id;
//     uint16_t service_start_handle;
//     uint16_t service_end_handle;
//     uint16_t char_handle;
//     esp_bd_addr_t remote_bda;
// };

// static struct gattc_profile_inst gl_profile_tab[PROFILE_NUM] = {
//     [PROFILE_A_APP_ID] = {
//         .gattc_cb = gattc_profile_event_handler,
//         .gattc_if = ESP_GATT_IF_NONE,
//     },
// };

// static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
// {
//     esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

//     switch (event)
//     {
//     case ESP_GATTC_REG_EVT:
//         ESP_LOGI(GATTC_TAG, "REG_EVT");
//         esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
//         if (scan_ret)
//         {
//             ESP_LOGE(GATTC_TAG, "set scan params error, error code = %x", scan_ret);
//         }
//         break;
//     case ESP_GATTC_CONNECT_EVT:
//     {
//         ESP_LOGI(GATTC_TAG, "ESP_GATTC_CONNECT_EVT conn_id %d, if %d", p_data->connect.conn_id, gattc_if);
//         gl_profile_tab[PROFILE_A_APP_ID].conn_id = p_data->connect.conn_id;
//         memcpy(gl_profile_tab[PROFILE_A_APP_ID].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
//         ESP_LOGI(GATTC_TAG, "REMOTE BDA:");
//         esp_log_buffer_hex(GATTC_TAG, gl_profile_tab[PROFILE_A_APP_ID].remote_bda, sizeof(esp_bd_addr_t));
//         esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req(gattc_if, p_data->connect.conn_id);
//         if (mtu_ret)
//         {
//             ESP_LOGE(GATTC_TAG, "config MTU error, error code = %x", mtu_ret);
//         }
//         break;
//     }
//     case ESP_GATTC_OPEN_EVT:
//         if (param->open.status != ESP_GATT_OK)
//         {
//             ESP_LOGE(GATTC_TAG, "open failed, status %d", p_data->open.status);
//             break;
//         }
//         ESP_LOGI(GATTC_TAG, "open success");
//         break;
//     case ESP_GATTC_DIS_SRVC_CMPL_EVT:
//         if (param->dis_srvc_cmpl.status != ESP_GATT_OK)
//         {
//             ESP_LOGE(GATTC_TAG, "discover service failed, status %d", param->dis_srvc_cmpl.status);
//             break;
//         }
//         ESP_LOGI(GATTC_TAG, "discover service complete conn_id %d", param->dis_srvc_cmpl.conn_id);
//         esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &remote_filter_service_uuid);
//         break;
//     case ESP_GATTC_CFG_MTU_EVT:
//         if (param->cfg_mtu.status != ESP_GATT_OK)
//         {
//             ESP_LOGE(GATTC_TAG, "config mtu failed, error status = %x", param->cfg_mtu.status);
//         }
//         ESP_LOGI(GATTC_TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
//         break;
//     case ESP_GATTC_SEARCH_RES_EVT:
//     {
//         ESP_LOGI(GATTC_TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
//         ESP_LOGI(GATTC_TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.end_handle, p_data->search_res.srvc_id.inst_id);
//         if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == REMOTE_SERVICE_UUID)
//         {
//             ESP_LOGI(GATTC_TAG, "service found");
//             get_server = true;
//             gl_profile_tab[PROFILE_A_APP_ID].service_start_handle = p_data->search_res.start_handle;
//             gl_profile_tab[PROFILE_A_APP_ID].service_end_handle = p_data->search_res.end_handle;
//             ESP_LOGI(GATTC_TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
//         }
//         break;
//     }
//     case ESP_GATTC_SEARCH_CMPL_EVT:
//         if (p_data->search_cmpl.status != ESP_GATT_OK)
//         {
//             ESP_LOGE(GATTC_TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
//             break;
//         }
//         if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_REMOTE_DEVICE)
//         {
//             ESP_LOGI(GATTC_TAG, "Get service information from remote device");
//         }
//         else if (p_data->search_cmpl.searched_service_source == ESP_GATT_SERVICE_FROM_NVS_FLASH)
//         {
//             ESP_LOGI(GATTC_TAG, "Get service information from flash");
//         }
//         else
//         {
//             ESP_LOGI(GATTC_TAG, "unknown service source");
//         }
//         ESP_LOGI(GATTC_TAG, "ESP_GATTC_SEARCH_CMPL_EVT");
//         if (get_server)
//         {
//             uint16_t count = 0;
//             esp_gatt_status_t status = esp_ble_gattc_get_attr_count(gattc_if,
//                                                                     p_data->search_cmpl.conn_id,
//                                                                     ESP_GATT_DB_CHARACTERISTIC,
//                                                                     gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
//                                                                     gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
//                                                                     INVALID_HANDLE,
//                                                                     &count);
//             if (status != ESP_GATT_OK)
//             {
//                 ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_attr_count error");
//             }

//             if (count > 0)
//             {
//                 char_elem_result = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
//                 if (!char_elem_result)
//                 {
//                     ESP_LOGE(GATTC_TAG, "gattc no mem");
//                 }
//                 else
//                 {
//                     status = esp_ble_gattc_get_char_by_uuid(gattc_if,
//                                                             p_data->search_cmpl.conn_id,
//                                                             gl_profile_tab[PROFILE_A_APP_ID].service_start_handle,
//                                                             gl_profile_tab[PROFILE_A_APP_ID].service_end_handle,
//                                                             remote_filter_char_uuid,
//                                                             char_elem_result,
//                                                             &count);
//                     if (status != ESP_GATT_OK)
//                     {
//                         ESP_LOGE(GATTC_TAG, "esp_ble_gattc_get_char_by_uuid error");
//                     }

//                     /*  Every service have only one char in our 'ESP_GATTS_DEMO' demo, so we used first 'char_elem_result' */
//                     if (count > 0 && (char_elem_result[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY))
//                     {
//                         gl_profile_tab[PROFILE_A_APP_ID].char_handle = char_elem_result[0].char_handle;

//                         esp_ble_gattc_read_char(gattc_if, p_data->search_cmpl.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle, ESP_GATT_AUTH_REQ_NONE);
//                     }
//                 }
//                 /* free char_elem_result */
//                 free(char_elem_result);
//             }
//             else
//             {
//                 ESP_LOGE(GATTC_TAG, "no char found");
//             }
//         }
//         break;
//     case ESP_GATTC_READ_CHAR_EVT:
//     {
//         ESP_LOGI(GATTC_TAG, "ESP_GATTC_READ_CHAR_EVT");
//         const char *char_pointer = (char *)p_data->read.value;
//         //  int battery_percentage = (int)*char_pointer;
//         ESP_LOGI(GATTC_TAG, "(p_data->read.value) Message: %s ", char_pointer);

//         vTaskDelay(5000 / portTICK_PERIOD_MS);
//         esp_ble_gattc_read_char(gattc_if, p_data->search_cmpl.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle, ESP_GATT_AUTH_REQ_NONE);

//         break;
//     }
//     case ESP_GATTC_SRVC_CHG_EVT:
//     {
//         esp_bd_addr_t bda;
//         memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
//         ESP_LOGI(GATTC_TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:");
//         esp_log_buffer_hex(GATTC_TAG, bda, sizeof(esp_bd_addr_t));
//         break;
//     }
//     case ESP_GATTC_DISCONNECT_EVT:
//         connect = false;
//         get_server = false;
//         ESP_LOGI(GATTC_TAG, "ESP_GATTC_DISCONNECT_EVT, reason = %d", p_data->disconnect.reason);
//         break;
//     default:
//         break;
//     }
// }

// static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
// {
//     uint8_t *adv_name = NULL;
//     uint8_t adv_name_len = 0;
//     switch (event)
//     {
//     case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
//     {
//         // the unit of the duration is second
//         uint32_t duration = 30;
//         esp_ble_gap_start_scanning(duration);
//         break;
//     }
//     case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
//         // scan start complete event to indicate scan start successfully or failed
//         if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
//         {
//             ESP_LOGE(GATTC_TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
//             break;
//         }
//         ESP_LOGI(GATTC_TAG, "scan start success");

//         break;
//     case ESP_GAP_BLE_SCAN_RESULT_EVT:
//     {
//         esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
//         switch (scan_result->scan_rst.search_evt)
//         {
//         case ESP_GAP_SEARCH_INQ_RES_EVT:
//             esp_log_buffer_hex(GATTC_TAG, scan_result->scan_rst.bda, 6);
//             ESP_LOGI(GATTC_TAG, "searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
//             adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
//                                                 ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
//             ESP_LOGI(GATTC_TAG, "searched Device Name Len %d", adv_name_len);
//             esp_log_buffer_char(GATTC_TAG, adv_name, adv_name_len);
//             ESP_LOGI(GATTC_TAG, "\n");

//             if (adv_name != NULL)
//             {
//                 if (strlen(remote_device_name) == adv_name_len && strncmp((char *)adv_name, remote_device_name, adv_name_len) == 0)
//                 {
//                     ESP_LOGI(GATTC_TAG, "searched device %s\n", remote_device_name);
//                     if (connect == false)
//                     {
//                         connect = true;
//                         ESP_LOGI(GATTC_TAG, "connect to the remote device.");
//                         esp_ble_gap_stop_scanning();
//                         esp_ble_gattc_open(gl_profile_tab[PROFILE_A_APP_ID].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
//                     }
//                 }
//             }
//             break;
//         case ESP_GAP_SEARCH_INQ_CMPL_EVT:
//             break;
//         default:
//             break;
//         }
//         break;
//     }

//     case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
//         if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
//         {
//             ESP_LOGE(GATTC_TAG, "scan stop failed, error status = %x", param->scan_stop_cmpl.status);
//             break;
//         }
//         ESP_LOGI(GATTC_TAG, "stop scan successfully");
//         break;

//     case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
//         if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
//         {
//             ESP_LOGE(GATTC_TAG, "adv stop failed, error status = %x", param->adv_stop_cmpl.status);
//             break;
//         }
//         ESP_LOGI(GATTC_TAG, "stop adv successfully");
//         break;
//     case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
//         ESP_LOGI(GATTC_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
//                  param->update_conn_params.status,
//                  param->update_conn_params.min_int,
//                  param->update_conn_params.max_int,
//                  param->update_conn_params.conn_int,
//                  param->update_conn_params.latency,
//                  param->update_conn_params.timeout);
//         break;
//     default:
//         break;
//     }
// }

// static void esp_gattc_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
// {
//     /* If event is register event, store the gattc_if for each profile */
//     if (event == ESP_GATTC_REG_EVT)
//     {
//         if (param->reg.status == ESP_GATT_OK)
//         {
//             gl_profile_tab[param->reg.app_id].gattc_if = gattc_if;
//         }
//         else
//         {
//             ESP_LOGI(GATTC_TAG, "reg app failed, app_id %04x, status %d",
//                      param->reg.app_id,
//                      param->reg.status);
//             return;
//         }
//     }

//     /* If the gattc_if equal to profile A, call profile A cb handler,
//      * so here call each profile's callback */
//     do
//     {
//         int idx;
//         for (idx = 0; idx < PROFILE_NUM; idx++)
//         {
//             if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
//                 gattc_if == gl_profile_tab[idx].gattc_if)
//             {
//                 if (gl_profile_tab[idx].gattc_cb)
//                 {
//                     gl_profile_tab[idx].gattc_cb(event, gattc_if, param);
//                 }
//             }
//         }
//     } while (0);
// }

// void getTemperatureAndHumidityFromMI()
// {
//     // UUID in char array format
//     //char uuid_char[] = "ebe0ccc1-7a0a-4b0c-8a1a-6ff2997da3a6";
//     char uuid_service[] = "ebe0ccb0-7a0a-4b0c-8a1a-6ff2997da3a6";

//     uint8_t uuid_char[16] = {
//         0xeb,
//         0xe0,
//         0xcc,
//         0xc1,
//         0x7a,
//         0x0a,
//         0x4b,
//         0x0c,
//         0x8a,
//         0x1a,
//         0x6f,
//         0xf2,
//         0x99,
//         0xfd,
//         0xa3,
//         0xa6,
//     };

//     uint8_t uuid_service[16] = {
//         0xeb,
//         0xe0,
//         0xcc,
//         0xb0,
//         0x7a,
//         0x0a,
//         0x4b,
//         0x0c,
//         0x8a,
//         0x1a,
//         0x6f,
//         0xf2,
//         0x99,
//         0xfd,
//         0xa3,
//         0xa6,
//     };

//     // // Convert UUID to uint8_t array
//     // uint8_t uuid_char_array[16];
//     // esp_ble_gap_utils_uuid_128_from_str(uuid_char, uuid_array);
//     // esp_ble_gap_config_adv_data_raw(uuid_array, sizeof(uuid_array));

//     // uint8_t uuid_service_array[16];
//     // esp_ble_gap_utils_uuid_128_from_str(uuid_service, uuid_service_array);
//     // esp_ble_gap_config_adv_data_raw(uuid_service_array, sizeof(uuid_service_array));

//     esp_bt_uuid_t remote_filter_service_uuid = {
//         .len = ESP_UUID_LEN_16,
//         .uuid = {
//             .uuid16 = uuid_service,
//         },
//     };

//     esp_bt_uuid_t remote_filter_char_uuid = {
//         .len = ESP_UUID_LEN_16,
//         .uuid = {
//             .uuid16 = uuid_char,
//         },
//     };

//     esp_ble_scan_params_t ble_scan_params = {
//         .scan_type = BLE_SCAN_TYPE_ACTIVE,
//         .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
//         .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
//         .scan_interval = 0x50,
//         .scan_window = 0x30,
//         .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE};

//     ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

//     esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
//     ret = esp_bt_controller_init(&bt_cfg);
//     if (ret)
//     {
//         ESP_LOGE(GATTC_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
//         return;
//     }

//     ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
//     if (ret)
//     {
//         ESP_LOGE(GATTC_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
//         return;
//     }

//     ret = esp_bluedroid_init();
//     if (ret)
//     {
//         ESP_LOGE(GATTC_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
//         return;
//     }

//     ret = esp_bluedroid_enable();
//     if (ret)
//     {
//         ESP_LOGE(GATTC_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
//         return;
//     }

//     // register the  callback function to the gap module
//     ret = esp_ble_gap_register_callback(esp_gap_cb);
//     if (ret)
//     {
//         ESP_LOGE(GATTC_TAG, "%s gap register failed, error code = %x\n", __func__, ret);
//         return;
//     }

//     // register the callback function to the gattc module
//     ret = esp_ble_gattc_register_callback(esp_gattc_cb);
//     if (ret)
//     {
//         ESP_LOGE(GATTC_TAG, "%s gattc register failed, error code = %x\n", __func__, ret);
//         return;
//     }

//     ret = esp_ble_gattc_app_register(PROFILE_A_APP_ID);
//     if (ret)
//     {
//         ESP_LOGE(GATTC_TAG, "%s gattc app register failed, error code = %x\n", __func__, ret);
//     }
//     esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
//     if (local_mtu_ret)
//     {
//         ESP_LOGE(GATTC_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
//     }
// }