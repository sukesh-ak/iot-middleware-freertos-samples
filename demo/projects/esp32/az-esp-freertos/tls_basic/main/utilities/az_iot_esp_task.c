/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Kernel includes. */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Demo Specific configs. */
#include "demo_config.h"
#include "az_iot_esp_task.h"

/* Azure Provisioning/IoTHub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_provisioning_client.h"

/* Exponential backoff retry include. */
#include "exponential_backoff.h"

/* Transport interface implementation include header for TLS. */
#include "tls_freertos.h"

/*-----------------------------------------------------------*/

/* Compile time error for undefined configs. */
#if !defined( democonfigHOSTNAME ) && !defined( democonfigENABLE_DPS_SAMPLE )
    #error "Define the config democonfigHOSTNAME by following the instructions in file demo_config.h."
#endif

#if !defined( democonfigENDPOINT ) && defined( democonfigENABLE_DPS_SAMPLE )
    #error "Define the config dps endpoint by following the instructions in file demo_config.h."
#endif

#ifndef democonfigROOT_CA_PEM
    #error "Please define Root CA certificate of the MQTT broker(democonfigROOT_CA_PEM) in demo_config.h."
#endif

#if defined( democonfigDEVICE_SYMMETRIC_KEY ) && defined( democonfigCLIENT_CERTIFICATE_PEM )
    #error "Please define only one auth democonfigDEVICE_SYMMETRIC_KEY or democonfigCLIENT_CERTIFICATE_PEM in demo_config.h."
#endif

#if !defined( democonfigDEVICE_SYMMETRIC_KEY ) && !defined( democonfigCLIENT_CERTIFICATE_PEM )
    #error "Please define one auth democonfigDEVICE_SYMMETRIC_KEY or democonfigCLIENT_CERTIFICATE_PEM in demo_config.h."
#endif

/* #TODO: Add value checks*/
/*-----------------------------------------------------------*/

/**
 * @brief Load the RootCA file with name noted at democonfigROOT_CA_PEM from KCONFIG loaded
 * into binary. Using asm for usage
*/
extern const uint8_t az_iothub_org_pem_start[] asm("_binary_az_root_ca_pem_start");
extern const uint8_t az_iothub_org_pem_end[]   asm("_binary_az_root_ca_pem_end");

/***************************************************************************************/

/*-----------------------------------------------------------*/

/* Define buffer for IoTHub info.  */
#ifdef democonfigENABLE_DPS_SAMPLE
    static uint8_t ucSampleIotHubHostname[ 128 ];
    static uint8_t ucSampleIotHubDeviceId[ 128 ];
    static AzureIoTProvisioningClient_t xAzureIoTProvisioningClient;
#endif /* democonfigENABLE_DPS_SAMPLE */

static void prvAzureDemoTask( void * pvParameters )
{
    uint32_t ulPublishCount = 0U;
    const uint32_t ulMaxPublishCount = 5UL;
    NetworkCredentials_t xNetworkCredentials = { 0 };
    AzureIoTTransportInterface_t xTransport;
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportStatus_t xNetworkStatus;
    TlsTransportParams_t xTlsTransportParams = { 0 };
    AzureIoTHubClientResult_t xResult;
    uint32_t ulStatus;
    AzureIoTHubClientOptions_t xHubOptions = { 0 };
    AzureIoTMessageProperties_t xPropertyBag;
    bool xSessionPresent;

    #ifdef democonfigENABLE_DPS_SAMPLE
        uint8_t * pucIotHubHostname = NULL;
        uint8_t * pucIotHubDeviceId = NULL;
        uint32_t pulIothubHostnameLength = 0;
        uint32_t pulIothubDeviceIdLength = 0;
    #else
        uint8_t * pucIotHubHostname = ( uint8_t * ) democonfigHOSTNAME;
        uint8_t * pucIotHubDeviceId = ( uint8_t * ) democonfigDEVICE_ID;
        uint32_t pulIothubHostnameLength = sizeof( democonfigHOSTNAME ) - 1;
        uint32_t pulIothubDeviceIdLength = sizeof( democonfigDEVICE_ID ) - 1;
    #endif /* democonfigENABLE_DPS_SAMPLE */

    ( void ) pvParameters;

    configASSERT( AzureIoT_Init() == eAzureIoTSuccess );

    ulStatus = prvSetupNetworkCredentials( &xNetworkCredentials );
    configASSERT( ulStatus == 0 );

    #ifdef democonfigENABLE_DPS_SAMPLE
        /* Run DPS.  */
        if( ( ulStatus = prvIoTHubInfoGet( &xNetworkCredentials, &pucIotHubHostname,
                                           &pulIothubHostnameLength, &pucIotHubDeviceId,
                                           &pulIothubDeviceIdLength ) ) != 0 )
        {
            LogError( ( "Failed on sample_dps_entry!: error code = 0x%08x\r\n", ulStatus ) );
            return;
        }
    #endif /* democonfigENABLE_DPS_SAMPLE */

    xNetworkContext.pParams = &xTlsTransportParams;

    for( ; ; )
    {
        /****************************** Connect. ******************************/

        /* Attempt to establish TLS session with MQTT broker. If connection fails,
         * retry after a timeout. Timeout value will be exponentially increased
         * until  the maximum number of attempts are reached or the maximum timeout
         * value is reached. The function returns a failure status if the TCP
         * connection cannot be established to the broker after the configured
         * number of attempts. */
        xNetworkStatus = prvConnectToServerWithBackoffRetries( ( const char * ) pucIotHubHostname,
                                                               democonfigMQTT_BROKER_PORT,
                                                               &xNetworkCredentials, &xNetworkContext );
        configASSERT( xNetworkStatus == TLS_TRANSPORT_SUCCESS );

        /* Fill in Transport Interface send and receive function pointers. */
        xTransport.pxNetworkContext = &xNetworkContext;
        xTransport.xSend = TLS_FreeRTOS_send;
        xTransport.xRecv = TLS_FreeRTOS_recv;

        /* Init IoT Hub option */
        xResult = AzureIoTHubClient_OptionsInit( &xHubOptions );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        xHubOptions.pucModuleID = ( const uint8_t * ) democonfigMODULE_ID;
        xHubOptions.ulModuleIDLength = sizeof( democonfigMODULE_ID ) - 1;

        /* Initialize MQTT library. */
        xResult = AzureIoTHubClient_Init( &xAzureIoTHubClient,
                                          pucIotHubHostname, pulIothubHostnameLength,
                                          pucIotHubDeviceId, pulIothubDeviceIdLength,
                                          &xHubOptions,
                                          ucSharedBuffer, sizeof( ucSharedBuffer ),
                                          prvGetUnixTime,
                                          &xTransport );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        #ifdef democonfigDEVICE_SYMMETRIC_KEY
            xResult = AzureIoTHubClient_SetSymmetricKey( &xAzureIoTHubClient,
                                                         ( const uint8_t * ) democonfigDEVICE_SYMMETRIC_KEY,
                                                         sizeof( democonfigDEVICE_SYMMETRIC_KEY ) - 1,
                                                         Crypto_HMAC );
            configASSERT( xResult == eAzureIoTHubClientSuccess );
        #endif // democonfigDEVICE_SYMMETRIC_KEY

        /* Sends an MQTT Connect packet over the already established TLS connection,
         * and waits for connection acknowledgment (CONNACK) packet. */
        LogInfo( ( "Creating an MQTT connection to %s.\r\n", pucIotHubHostname ) );

        xResult = AzureIoTHubClient_Connect( &xAzureIoTHubClient,
                                             false, &xSessionPresent,
                                             sampleazureiotCONNACK_RECV_TIMEOUT_MS );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        /**************************** Enable features. ******************************/

        xResult = AzureIoTHubClient_SubscribeCloudToDeviceMessage( &xAzureIoTHubClient, prvHandleCloudMessage,
                                                                   &xAzureIoTHubClient, sampleazureiotWAIT_FOREVER );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        xResult = AzureIoTHubClient_SubscribeDirectMethod( &xAzureIoTHubClient, prvHandleDirectMethod,
                                                           &xAzureIoTHubClient, sampleazureiotWAIT_FOREVER );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        xResult = AzureIoTHubClient_SubscribeDeviceTwin( &xAzureIoTHubClient, prvHandleDeviceTwinMessage,
                                                         &xAzureIoTHubClient, sampleazureiotWAIT_FOREVER );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        /* Get the device twin on boot */
        xResult = AzureIoTHubClient_GetDeviceTwin( &xAzureIoTHubClient );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        /* Create a bag of properties for the telemetry */
        xResult = AzureIoT_MessagePropertiesInit( &xPropertyBag, ucPropertyBuffer, 0, sizeof( xPropertyBag ) );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        xResult = AzureIoT_MessagePropertiesAppend( &xPropertyBag, ( uint8_t * ) "name", sizeof( "name" ) - 1,
                                                    ( uint8_t * ) "value", sizeof( "value" ) - 1 );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        /****************** Publish and Keep Alive Loop. **********************/
        /* Publish messages with QoS1, send and process Keep alive messages. */
        for( ulPublishCount = 0; ulPublishCount < ulMaxPublishCount; ulPublishCount++ )
        {
            xResult = AzureIoTHubClient_SendTelemetry( &xAzureIoTHubClient,
                                                       ( const uint8_t * ) sampleazureiotMESSAGE,
                                                       sizeof( sampleazureiotMESSAGE ) - 1,
                                                       &xPropertyBag );
            configASSERT( xResult == eAzureIoTHubClientSuccess );

            LogInfo( ( "Attempt to receive publish message from IoTHub.\r\n" ) );
            xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient,
                                                     sampleazureiotPROCESS_LOOP_TIMEOUT_MS );
            configASSERT( xResult == eAzureIoTHubClientSuccess );

            if( ulPublishCount % 2 == 0 )
            {
                /* Send reported property every other cycle */
                xResult = AzureIoTHubClient_SendDeviceTwinReported( &xAzureIoTHubClient,
                                                                    ( const uint8_t * ) sampleazureiotTWIN_PROPERTY,
                                                                    sizeof( sampleazureiotTWIN_PROPERTY ) - 1,
                                                                    NULL );
                configASSERT( xResult == eAzureIoTHubClientSuccess );
            }

            /* Leave Connection Idle for some time. */
            LogInfo( ( "Keeping Connection Idle...\r\n\r\n" ) );
            vTaskDelay( sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS );
        }

        /**************************** Disconnect. *****************************/

        xResult = AzureIoTHubClient_UnsubscribeDeviceTwin( &xAzureIoTHubClient );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        xResult = AzureIoTHubClient_UnsubscribeDirectMethod( &xAzureIoTHubClient );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        xResult = AzureIoTHubClient_UnsubscribeCloudToDeviceMessage( &xAzureIoTHubClient );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        /* Send an MQTT Disconnect packet over the already connected TLS over
         * TCP connection. There is no corresponding response for the disconnect
         * packet. After sending disconnect, client must close the network
         * connection. */
        xResult = AzureIoTHubClient_Disconnect( &xAzureIoTHubClient );
        configASSERT( xResult == eAzureIoTHubClientSuccess );

        /* Close the network connection.  */
        TLS_FreeRTOS_Disconnect( &xNetworkContext );

        /* Wait for some time between two iterations to ensure that we do not
         * bombard the broker. */
        LogInfo( ( "Demo completed successfully.\r\n" ) );
        LogInfo( ( "Short delay before starting the next iteration.... \r\n\r\n" ) );
        vTaskDelay( sampleazureiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS );
    }
}

#ifdef democonfigENABLE_DPS_SAMPLE

    static uint32_t prvIoTHubInfoGet( NetworkCredentials_t * pXNetworkCredentials,
                                      uint8_t ** ppucIothubHostname,
                                      uint32_t * pulIothubHostnameLength,
                                      uint8_t ** ppucIothubDeviceId,
                                      uint32_t * pulIothubDeviceIdLength )
    {
        NetworkContext_t xNetworkContext = { 0 };
        TlsTransportParams_t xTlsTransportParams = { 0 };
        TlsTransportStatus_t xNetworkStatus;
        AzureIoTProvisioningClientResult_t xResult;
        AzureIoTTransportInterface_t xTransport;
        uint32_t ucSamplepIothubHostnameLength = sizeof( ucSampleIotHubHostname );
        uint32_t ucSamplepIothubDeviceIdLength = sizeof( ucSampleIotHubDeviceId );

        /* Set the pParams member of the network context with desired transport. */
        xNetworkContext.pParams = &xTlsTransportParams;

        /****************************** Connect. ******************************/

        xNetworkStatus = prvConnectToServerWithBackoffRetries( democonfigENDPOINT, democonfigMQTT_BROKER_PORT,
                                                               pXNetworkCredentials, &xNetworkContext );
        configASSERT( xNetworkStatus == TLS_TRANSPORT_SUCCESS );

        /* Fill in Transport Interface send and receive function pointers. */
        xTransport.pxNetworkContext = &xNetworkContext;
        xTransport.xSend = TLS_FreeRTOS_send;
        xTransport.xRecv = TLS_FreeRTOS_Recv;

        /* Initialize MQTT library. */
        xResult = AzureIoTProvisioningClient_Init( &xAzureIoTProvisioningClient,
                                                   ( const uint8_t * ) democonfigENDPOINT,
                                                   sizeof( democonfigENDPOINT ) - 1,
                                                   ( const uint8_t * ) democonfigID_SCOPE,
                                                   sizeof( democonfigID_SCOPE ) - 1,
                                                   ( const uint8_t * ) democonfigREGISTRATION_ID,
                                                   sizeof( democonfigREGISTRATION_ID ) - 1,
                                                   NULL, ucSharedBuffer, sizeof( ucSharedBuffer ),
                                                   prvGetUnixTime,
                                                   &xTransport );
        configASSERT( xResult == eAzureIoTProvisioningSuccess );

        #ifdef democonfigDEVICE_SYMMETRIC_KEY
            xResult = AzureIoTProvisioningClient_SetSymmetricKey( &xAzureIoTProvisioningClient,
                                                                  ( const uint8_t * ) democonfigDEVICE_SYMMETRIC_KEY,
                                                                  sizeof( democonfigDEVICE_SYMMETRIC_KEY ) - 1,
                                                                  Crypto_HMAC );
            configASSERT( xResult == eAzureIoTProvisioningSuccess );
        #endif // democonfigDEVICE_SYMMETRIC_KEY

        do
        {
            xResult = AzureIoTProvisioningClient_Register( &xAzureIoTProvisioningClient,
                                                           sampleazureiotProvisioning_Registration_TIMEOUT_MS );
        } while( xResult == eAzureIoTProvisioningPending );

        configASSERT( xResult == eAzureIoTProvisioningSuccess );

        xResult = AzureIoTProvisioningClient_GetDeviceAndHub( &xAzureIoTProvisioningClient,
                                                              ucSampleIotHubHostname, &ucSamplepIothubHostnameLength,
                                                              ucSampleIotHubDeviceId, &ucSamplepIothubDeviceIdLength );
        configASSERT( xResult == eAzureIoTProvisioningSuccess );

        AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );

        /* Close the network connection.  */
        TLS_FreeRTOS_Disconnect( &xNetworkContext );

        *ppucIothubHostname = ucSampleIotHubHostname;
        *pulIothubHostnameLength = ucSamplepIothubHostnameLength;
        *ppucIothubDeviceId = ucSampleIotHubDeviceId;
        *pulIothubDeviceIdLength = ucSamplepIothubDeviceIdLength;

        return 0;
    }

#endif // democonfigENABLE_DPS_SAMPLE
/*-----------------------------------------------------------*/

static void prvHandleCloudMessage( AzureIoTHubClientCloudToDeviceMessageRequest_t * pxMessage,
                                   void * pvContext )
{
    ( void ) pvContext;

    LogInfo( ( "Cloud message payload : %.*s \r\n",
               pxMessage->ulPayloadLength,
               pxMessage->pvMessagePayload ) );
}

/*-----------------------------------------------------------*/

static void prvHandleDirectMethod( AzureIoTHubClientMethodRequest_t * pxMessage,
                                   void * pvContext )
{
    LogInfo( ( "Method payload : %.*s \r\n",
               pxMessage->ulPayloadLength,
               pxMessage->pvMessagePayload ) );

    AzureIoTHubClient_t * xHandle = ( AzureIoTHubClient_t * ) pvContext;

    if( AzureIoTHubClient_SendMethodResponse( xHandle, pxMessage, 200,
                                              NULL, 0 ) != eAzureIoTHubClientSuccess )
    {
        LogInfo( ( "Error sending method response\r\n" ) );
    }
}

/*-----------------------------------------------------------*/
static void prvHandleDeviceTwinMessage( AzureIoTHubClientTwinResponse_t * pxMessage,
                                        void * pvContext )
{
    ( void ) pvContext;

    switch( pxMessage->xMessageType )
    {
        case eAzureIoTHubTwinGetMessage:
            LogInfo( ( "Device twin document GET received" ) );
            break;

        case eAzureIoTHubTwinReportedResponseMessage:
            LogInfo( ( "Device twin reported property response received" ) );
            break;

        case eAzureIoTHubTwinDesiredPropertyMessage:
            LogInfo( ( "Device twin desired property received" ) );
            break;

        default:
            LogError( ( "Unknown twin message" ) );
    }

    LogInfo( ( "Twin document payload : %.*s \r\n",
               pxMessage->ulPayloadLength,
               pxMessage->pvMessagePayload ) );
}

/*-----------------------------------------------------------*/
static TlsTransportStatus_t prvConnectToServerWithBackoffRetries( const char * pcHostName,
                                                                  uint32_t port,
                                                                  NetworkCredentials_t * pxNetworkCredentials,
                                                                  NetworkContext_t * pxNetworkContext )
{
    TlsTransportStatus_t xNetworkStatus;
    RetryUtilsStatus_t xRetryUtilsStatus = RetryUtilsSuccess;
    RetryUtilsParams_t xReconnectParams;

    /* Initialize reconnect attempts and interval. */
    RetryUtils_ParamsReset( &xReconnectParams );
    xReconnectParams.maxRetryAttempts = MAX_RETRY_ATTEMPTS;

    /* Attempt to connect to MQTT broker. If connection fails, retry after
     * a timeout. Timeout value will exponentially increase till maximum
     * attempts are reached.
     */
    do
    {
        /* Establish a TLS session with the MQTT broker. This example connects to
         * the MQTT broker as specified in democonfigHOSTNAME and
         * democonfigMQTT_BROKER_PORT at the top of this file. */
        LogInfo( ( "Creating a TLS connection to %s:%u.\r\n", pcHostName, port ) );
         /* Attempt to create a server-authenticated TLS connection. */
        xNetworkStatus = TLS_FreeRTOS_Connect(  pxNetworkContext,
                                                pcHostName, port,
                                                pxNetworkCredentials,
                                                sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS,
                                                sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS );

        if( xNetworkStatus != TLS_TRANSPORT_SUCCESS )
        {
            /* Generate a random number and calculate backoff value (in milliseconds) for
             * the next connection retry.
             * Note: It is recommended to seed the random number generator with a device-specific
             * entropy source so that possibility of multiple devices retrying failed network operations
             * at similar intervals can be avoided. */
            LogWarn( ( "Connection to the broker failed. Retrying connection with backoff and jitter." ) );
            xRetryUtilsStatus = RetryUtils_BackoffAndSleep( &xReconnectParams );

            if( xRetryUtilsStatus == RetryUtilsRetriesExhausted )
            {
                LogError( ( "Connection to the broker failed, all attempts exhausted." ) );
            }
            else if( xRetryUtilsStatus == RetryUtilsSuccess )
            {
                LogWarn( ( "Connection to the broker failed. "
                           "Retrying connection with backoff and jitter." ) );
                vTaskDelay( pdMS_TO_TICKS( usNextRetryBackOff ) );
            }
        }
    } while( ( xNetworkStatus != TLS_TRANSPORT_SUCCESS ) && ( xRetryUtilsStatus == RetryUtilsSuccess ) );

    return xNetworkStatus;
}

/*-----------------------------------------------------------*/

static uint64_t prvGetUnixTime( void )
{
    TickType_t xTickCount = 0;
    uint64_t ulTime = 0UL;

    /* Get the current tick count. */
    xTickCount = xTaskGetTickCount();

    /* Convert the ticks to milliseconds. */
    ulTime = ( uint64_t ) xTickCount / configTICK_RATE_HZ;

    /* Reduce ulGlobalEntryTimeMs from obtained time so as to always return the
     * elapsed time in the application. */
    ulTime = ( uint64_t ) ( ulTime + ulGlobalEntryTime );

    return ulTime;
}

/*-----------------------------------------------------------*/

/**************************************************************************/
static uint32_t prvSetupNetworkCredentials( NetworkCredentials_t * pxNetworkCredentials )
{
    pxNetworkCredentials->disableSni = democonfigDISABLE_SNI;
    /* Set the credentials for establishing a TLS connection. */
    pxNetworkCredentials->pRootCa = ( const unsigned char * ) az_iothub_org_pem_start;
    pxNetworkCredentials->rootCaSize = az_iothub_org_pem_end - az_iothub_org_pem_start;
    #ifdef democonfigCLIENT_CERTIFICATE_PEM //Change this later to use KConfig and asm binary 
        pxNetworkCredentials->pClientCert = ( const unsigned char * ) democonfigCLIENT_CERTIFICATE_PEM;
        pxNetworkCredentials->clientCertSize = sizeof( democonfigCLIENT_CERTIFICATE_PEM );
        pxNetworkCredentials->pPrivateKey = ( const unsigned char * ) democonfigCLIENT_PRIVATE_KEY_PEM;
        pxNetworkCredentials->privateKeySize = sizeof( democonfigCLIENT_PRIVATE_KEY_PEM );
    #endif

    return 0;
}

