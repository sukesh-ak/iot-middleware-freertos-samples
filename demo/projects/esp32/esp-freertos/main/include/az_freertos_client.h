// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _AZ_FREERTOS_CLIENT_H_
#define _AZ_FREERTOS_CLIENT_H_

/********************************************************************************
 * Name:            az_freertos_client.h
 * Description:     @brief Header file for Azure IoT Task for Telemetry send 
 *                  for Wi-Fi, NVS, & Config
 * Initial Date:    05.15.2021
*********************************************************************************/

/* ESP Config System */
#include "sdkconfig.h"

/* ESP LWIP and TLS libraries */
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "esp_tls.h"
#include "mqtt_client"

/* Azure Provisioning/IoTHub library includes */
#include "azure_iot_hub_client.h"

/* Application transport wrapper for Azure IoT Hub */
#include "az_esp_transport_wrapper.h"


/**
 * @brief The maximum number of retries for network operation with server.
 */
#define sampleazureiotRETRY_MAX_ATTEMPTS                      ( 5U )

/**
 * @brief The maximum back-off delay (in milliseconds) for retrying failed operation
 *  with server.
 */
#define sampleazureiotRETRY_MAX_BACKOFF_DELAY_MS              ( 5000U )

/**
 * @brief The base back-off delay (in milliseconds) to use for network operation retry
 * attempts.
 */
#define sampleazureiotRETRY_BACKOFF_BASE_MS                   ( 500U )

/**
 * @brief Timeout for receiving CONNACK packet in milliseconds.
 */
#define sampleazureiotCONNACK_RECV_TIMEOUT_MS                 ( 1000U )

/**
 * @brief The Telemetry message published in this example.
 */
#define sampleazureiotMESSAGE                                 "Hello World!"

/**
 * @brief The reported property payload to send to IoT Hub
 */
#define sampleazureiotTWIN_PROPERTY                           "{ \"hello\": \"world\" }"

/**
 * @brief Time in ticks to wait between each cycle of the demo implemented
 * by prvMQTTDemoTask().
 */
#define sampleazureiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS     ( pdMS_TO_TICKS( 5000U ) )

/**
 * @brief Timeout for MQTT_ProcessLoop in milliseconds.
 */
#define sampleazureiotPROCESS_LOOP_TIMEOUT_MS                 ( 500U )

/**
 * @brief Delay (in ticks) between consecutive cycles of MQTT publish operations in a
 * demo iteration.
 *
 * Note that the process loop also has a timeout, so the total time between
 * publishes is the sum of the two delays.
 */
#define sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS           ( pdMS_TO_TICKS( 2000U ) )

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS          ( 2000U )

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define sampleazureiotProvisioning_Registration_TIMEOUT_MS    ( 20U )

/**
 * @brief Use for blocking the call forever.
 */
#define sampleazureiotWAIT_FOREVER                            ( uint32_t )0xFFFFFFFF

/*-----------------------------------------------------------*/

/**
 * @brief Unix time.
 *
 * @return Time in milliseconds.
 */
static uint64_t prvGetUnixTime( void );

/*-----------------------------------------------------------*/

/**
 * @brief Static buffer used to hold MQTT messages being sent and received.
 */
static uint8_t ucSharedBuffer[ democonfigNETWORK_BUFFER_SIZE ];

/**
 * @brief Global start unix time
 */
static uint64_t ulGlobalEntryTime = 1639093301;

/*-----------------------------------------------------------*/



static void prvHandleCloudMessage( AzureIoTHubClientCloudToDeviceMessageRequest_t * pxMessage,
                                   void * pvContext );

static void prvHandleDirectMethod( AzureIoTHubClientMethodRequest_t * pxMessage,
void * pvContext );

static void prvHandleDirectMethod( AzureIoTHubClientMethodRequest_t * pxMessage,
                                   void * pvContext );

static void prvHandleDeviceTwinMessage( AzureIoTHubClientTwinResponse_t * pxMessage,
    void * pvContext );

static uint64_t prvGetUnixTime( void );


// /**
//  * @brief Connect to MQTT broker with reconnection retries.
//  *
//  * If connection fails, retry is attempted after a timeout.
//  * Timeout value will exponentially increase until maximum
//  * timeout value is reached or the number of attempts are exhausted.
//  *
//  * @param[out] pxNetworkContext The parameter to return the created network context.
//  *
//  * @return The status of the final connection attempt.
//  */
// static TlsTransportStatus_t prvConnectToServerWithBackoffRetries( const char * pcHostName,
//                                                                   uint32_t ulPort,
//                                                                   NetworkCredentials_t * pxNetworkCredentials,
//                                                                   NetworkContext_t * pxNetworkContext );


#endif