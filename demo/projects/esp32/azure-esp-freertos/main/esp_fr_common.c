#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "esp_fr_common.h"


//Declare static prototype
static esp_err_t esp32_event_handler(void *ctx, system_event_t *event);

//WIFI settings
static EventGroupHandle_t wifi_event_grp;
static int wifi_retry_num = 0;
static ip4_addr_t* esp32_ip_addr;


/******************************************************************************
 * FunctionName : esp32_event_handler
 * Description  : Callback to handle base level events from the ESP32 
 *                System Start, WIFI, etc. 
 * Parameters   : *ctx (void) 
 *                *event (system_event_t)
 * Returns      : esp_err_t //TODO: updated to latest 4.2.1 API but needs clearup
*******************************************************************************/

static void esp32_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        if (wifi_retry_num < ESP_WIFI_MAXIMUM_RETRY) 
        {
            esp_wifi_connect();
            wifi_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else 
        {
            xEventGroupSetBits(wifi_event_grp, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_retry_num = 0;
        xEventGroupSetBits(wifi_event_grp, WIFI_CONNECTED_BIT);
    }
}


/******************************************************************************
 * FunctionName : nvs_init
 * Description  : Initialize NVS
 * Parameters   : none
 * Returns      : esp_err_t
*******************************************************************************/
esp_err_t nvs_init()
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || 
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    printf("NVS is initialized\n");
    return ESP_OK; 
}

/******************************************************************************
 * FunctionName : wifi_init
 * Description  : Initialize WiFI
 * Parameters   : none
 * Returns      : esp_err_t
*******************************************************************************/
esp_err_t wifi_init()
{
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    EventBits_t wait_bits;

    //Initialize WiFi
    wifi_event_grp = xEventGroupCreate();

    //tcpip_adapter_init(); ESP IDF 4.1 and lower
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ESP_WIFI_SSID,
            .password = ESP_WIFI_PASS
        },
    };   

    //Check for errors
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    

    wait_bits = xEventGroupWaitBits(wifi_event_grp, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);


    ESP_LOGI(TAG, "wifi_init finished.");
    ESP_LOGI(TAG, "Connected to WiFi AP-SSID:%s password:%s",
             ESP_WIFI_SSID, ESP_WIFI_PASS);

    if (wait_bits & WIFI_CONNECTED_BIT) 
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
    } 
    else if (wait_bits & WIFI_FAIL_BIT) 
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 ESP_WIFI_SSID, ESP_WIFI_PASS);
        return ESP_FAIL;
    } 
    else 
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
 
    return ESP_OK;
}

/******************************************************************************
 * FunctionName : get_esp_rand_num
 * Description  : Use ESP random number generator
 * Parameters   : none
 * Returns      : uint32_t 
*******************************************************************************/

UBaseType_t get_esp_rand_num( void )
{
    return esp_random();
}