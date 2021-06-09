#ifndef AZURE_IOT_MQTT_PORT_H
#define AZURE_IOT_MQTT_PORT_H

#include <stdio.h>

#include "mqtt_client.h"
#include "esp_tls.h"

/* Maps MQTTContext directly to AzureIoTMQTT */
typedef struct esp_mqtt_client_handle_t  AzureIoTMQTT_t;

AzureIoTMQTTResult_t AzureIoTMQTT_ESP_Init( AzureIoTMQTTHandle_t xContext,
                                        const AzureIoTTransportInterface_t * pxTransportInterface,
                                        AzureIoTMQTTGetCurrentTimeFunc_t xGetTimeFunction,
                                        AzureIoTMQTTEventCallback_t xUserCallback,
                                        uint8_t * pucNetworkBuffer,
                                        size_t xNetworkBufferLength );

#endif // AZURE_IOT_MQTT_PORT_H
