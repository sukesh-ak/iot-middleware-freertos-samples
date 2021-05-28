/*-*****-*****-*****-*****-*****-*****-*****-*****-*****-*****-*****-*****-*****-*****
 * Author:          Microsoft - Azure IoT                                            *
 * Project:         Azure IoT Middleware for FreeRTOS                                *
 * Artifact Name:   esp-freertos-sample                                                     *
 * Artifact Desc:   Sample MQTT appplication for ESP32 with Azure                    *
 *                                                                                   *
 * //Legal Stuff goes here//                                                             *   
 *-*****-*****-*****-*****-*****-*****-*****-*****-*****-*****-*****-*****-*****-*****/



/******************************************************************************
 * Name:            main.c
 * Description:     Source for entry point to application which 
 *                  send sensor telemetry and heap information to Azure
 *                  from ESP32 device using Azure IoT Middleware
 * Initial Date:    05.15.2021
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "sdkconfig.h"

#include "esp_hw_common.h"


extern const uint8_t az_mqtt_pem_start[] asm("_binary_mqtt_eclipse_org_pem_start");
extern const uint8_t az_mqtt_pem_end[]   asm("_binary_mqtt_eclipse_org_pem_end");

/******************************************************************************
 * FunctionName : board_init
 * Description  : @brief Call functions to initialize ESP32 NVS, TCPIP Adapter/WIFI
 *                and Device Sensors (if applicable)
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
 * FunctionName : az_freertos_task
 * Description  : @brief Task for sending data to Azure 
 * Parameters   : pvParameter
 * Returns      : none
*******************************************************************************/
void az_freertos_task(void *pvParameter)
{
    char device_message[128]; 
    int counter = 0;
    
    TickType_t msgdelay = ONE_SECOND_DELAY *5;
    
    while (1)
    {
       //Get Heap Info 
       free_heap_sz = esp_get_free_heap_size();
       min_free_heap_sz = esp_get_minimum_free_heap_size();
       ESP_LOGI(TAG, "Free Heap Size: %lu", (unsigned long)free_heap_sz);
       ESP_LOGI(TAG, "Minimum Free Heap Size: %lu", (unsigned long)min_free_heap_sz);

       
        /* Create message to send to Azure IoT  - TEW*/
        snprintf(device_message, sizeof(device_message),
              "{\"MessageNum\": %d, \"FreeHeap\": %lu, \"Temperature\": %.1f, \"Humidity\": %.1f, \"Luminance\": %.1f}", 
                 ++counter, (unsigned long)free_heap_sz, temperature,
                 humidity, luminance);
        
        //Send message to Azure IoT Hub
        int sendresult = send_azure_msgs(device_message, azure_client);
        if (sendresult == 0)
        {
            printf("APP INFO: Azure Message Sent Successfully\n");
            ESP_LOGI(TAG, "SUCCESS: Message Queued to be sent to Azure");
        }
        else
        {
            printf("APP INFO: Error - Message not sent successfully\n");
            ESP_LOGE(TAG, "ERROR: Message Not Queued for Azure");
        }

        //Process Any Messages
        process_azure_msgs(azure_client);
        vTaskDelay(ONE_SECOND_DELAY *2);

        //Delay for 5 seconds
        vTaskDelay(msgdelay);     
    }
    
    //Clean up Azure IoT Connection
    destroy_iothub_client(azure_client);

    vTaskDelete(NULL);
}

/******************************************************************************
 * FunctionName : app_main
 * Description  : @brief entry of user application, init user function here
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
    taskResult = xTaskCreate(&az_freertos_task, "az_freertos_task", 1024 * 6, NULL, 5, NULL); 
    if (taskResult != pdPASS)
    {   
        ESP_LOGE(TAG, "Create azure task failed"); 
    }

    //Low Medium Priority Task
    //TODO: You can add other tasks here

}
