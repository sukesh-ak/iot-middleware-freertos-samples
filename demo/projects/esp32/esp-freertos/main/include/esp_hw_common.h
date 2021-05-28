#ifndef _ESP_HW_COMMON_H_
#define _ESP_HW_COMMON_H_

/********************************************************************************
 * Name:            esp_hw_common.h
 * Description:     @brief Header file for Common ESP32 HW init functions 
 *                  for Wi-Fi, NVS, & Config
 * Initial Date:    05.15.2021
*********************************************************************************/

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "sdkconfig.h"

/* Variable Declaration */
#define WIFI_CONNECTED_BIT      BIT0
#define ESP_WIFI_SSID           CONFIG_WIFI_SSID
#define ESP_WIFI_PASS           CONFIG_WIFI_PASSWORD
#define ESP_WIFI_MAXIMUM_RETRY  CONFIG_WIFI_MAXIMUM_RETRY
#define AZ_MQTT_BROKER_URI      CONFIG_AZ_BROKER_URI
#define ONE_SECOND_DELAY        (1000 / portTICK_PERIOD_MS)

static const char *TAG = "ESP_HARDWARE_CONFIG";
esp_err_t nvs_init();
esp_err_t wifi_init();

#endif