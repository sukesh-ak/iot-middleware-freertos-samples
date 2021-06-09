/**
 * Name:            main.c
 * Description:     Source for entry point to application which 
 *                  send sensor telemetry and heap information to Azure
 *                  from ESP32 device
 * Initial Date:    05.31.2021
**/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "sdkconfig.h"

#include "esp_fr_common.h"
//#include "azure_hub_client.h"

/* Azure Provisioning/IoTHub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_provisioning_client.h"

/* Exponential backoff retry include. */
#include "backoff_algorithm.h"


/******************************************************************************
 * FunctionName : board_init
 * Description  : Call functions to initialize ESP32 NVS, TCPIP Adapter/WIFI
 *                and Device Sensors
 * Parameters   : none
 * Returns      : void
*******************************************************************************/
void board_init()
{
    esp_err_t retvalue;
    retvalue = nvs_init();
    ESP_ERROR_CHECK(retvalue);

    retvalue = wifi_init();
    ESP_ERROR_CHECK(retvalue);

}


/******************************************************************************
 * FunctionName : app_main
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void app_main()
{
    int taskResult;
    
    board_init();
    vTaskDelay(ONE_SECOND_DELAY);
    ESP_LOGI(TAG, "ESP32 Board Initialized...");
   
   //High Priority Task
    taskResult = xTaskCreate(&prvAzureDemoTask, "prvAzureDemoTask", 1024 * 6, NULL, 5, NULL); 
    if (taskResult != pdPASS)
    {   
        ESP_LOGE(TAG, "Create azure task failed"); 
    }

}

