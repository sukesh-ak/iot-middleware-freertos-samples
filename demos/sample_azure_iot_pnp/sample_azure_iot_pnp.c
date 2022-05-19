/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Azure Provisioning/IoT Hub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_hub_client_properties.h"
#include "azure_iot_provisioning_client.h"

/* Azure JSON includes */
#include "azure_iot_json_reader.h"
#include "azure_iot_json_writer.h"

/* Exponential backoff retry include. */
#include "backoff_algorithm.h"

/* Transport interface implementation include header for TLS. */
#include "transport_tls_socket.h"

/* Crypto helper header. */
#include "crypto.h"

#include "mbedtls/base64.h"

/* Demo Specific configs. */
#include "demo_config.h"

/* Data Interface Definition */
#include "sample_azure_iot_pnp_data_if.h"
/*-----------------------------------------------------------*/

/* Compile time error for undefined configs. */
#if !defined( democonfigHOSTNAME ) && !defined( democonfigENABLE_DPS_SAMPLE )
    #error "Define the config democonfigHOSTNAME by following the instructions in file demo_config.h."
#endif

#if !defined( democonfigENDPOINT ) && defined( democonfigENABLE_DPS_SAMPLE )
    #error "Define the config dps endpoint by following the instructions in file demo_config.h."
#endif

#ifndef democonfigROOT_CA_PEM
    #error "Please define Root CA certificate of the IoT Hub(democonfigROOT_CA_PEM) in demo_config.h."
#endif

#if defined( democonfigDEVICE_SYMMETRIC_KEY ) && defined( democonfigCLIENT_CERTIFICATE_PEM )
    #error "Please define only one auth democonfigDEVICE_SYMMETRIC_KEY or democonfigCLIENT_CERTIFICATE_PEM in demo_config.h."
#endif

#if !defined( democonfigDEVICE_SYMMETRIC_KEY ) && !defined( democonfigCLIENT_CERTIFICATE_PEM )
    #error "Please define one auth democonfigDEVICE_SYMMETRIC_KEY or democonfigCLIENT_CERTIFICATE_PEM in demo_config.h."
#endif
/*-----------------------------------------------------------*/

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
#define sampleazureiotCONNACK_RECV_TIMEOUT_MS                 ( 10 * 1000U )

/**
 * @brief Date-time to use for the model id
 */
#define sampleazureiotDATE_TIME_FORMAT                        "%Y-%m-%dT%H:%M:%S.000Z"

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
#define sampleazureiotProvisioning_Registration_TIMEOUT_MS    ( 3 * 1000U )

/**
 * @brief Wait timeout for subscribe to finish.
 */
#define sampleazureiotSUBSCRIBE_TIMEOUT                       ( 10 * 1000U )
/*-----------------------------------------------------------*/

/**
 * @brief Unix time.
 *
 * @return Time in milliseconds.
 */
uint64_t ullGetUnixTime( void );
/*-----------------------------------------------------------*/

/* Define buffer for IoT Hub info.  */
#ifdef democonfigENABLE_DPS_SAMPLE
    static uint8_t ucSampleIotHubHostname[ 128 ];
    static uint8_t ucSampleIotHubDeviceId[ 128 ];
    static AzureIoTProvisioningClient_t xAzureIoTProvisioningClient;
#endif /* democonfigENABLE_DPS_SAMPLE */

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    TlsTransportParams_t * pParams;
};

AzureIoTHubClient_t xAzureIoTHubClient;

/* Telemetry buffers */
static uint8_t ucScratchBuffer[ 512 ];

/* Command buffers */
static uint8_t ucCommandResponsePayloadBuffer[ 256 ];

/* Reported Properties buffers */
static uint8_t ucReportedPropertiesUpdate[ 380 ];
static uint32_t ulReportedPropertiesUpdateLength;
/*-----------------------------------------------------------*/

#ifdef democonfigENABLE_DPS_SAMPLE

/**
 * @brief Gets the IoT Hub endpoint and deviceId from Provisioning service.
 *   This function will block for Provisioning service for result or return failure.
 *
 * @param[in] pXNetworkCredentials  Network credential used to connect to Provisioning service
 * @param[out] ppucIothubHostname  Pointer to uint8_t* IoT Hub hostname return from Provisioning Service
 * @param[in,out] pulIothubHostnameLength  Length of hostname
 * @param[out] ppucIothubDeviceId  Pointer to uint8_t* deviceId return from Provisioning Service
 * @param[in,out] pulIothubDeviceIdLength  Length of deviceId
 */
    static uint32_t prvIoTHubInfoGet( NetworkCredentials_t * pXNetworkCredentials,
                                      uint8_t ** ppucIothubHostname,
                                      uint32_t * pulIothubHostnameLength,
                                      uint8_t ** ppucIothubDeviceId,
                                      uint32_t * pulIothubDeviceIdLength );

#endif /* democonfigENABLE_DPS_SAMPLE */

/**
 * @brief The task used to demonstrate the Azure IoT Hub API.
 *
 * @param[in] pvParameters Parameters as passed at the time of task creation. Not
 * used in this example.
 */
static void prvAzureDemoTask( void * pvParameters );

/**
 * @brief Connect to endpoint with reconnection retries.
 *
 * If connection fails, retry is attempted after a timeout.
 * Timeout value will exponentially increase until maximum
 * timeout value is reached or the number of attempts are exhausted.
 *
 * @param pcHostName Hostname of the endpoint to connect to.
 * @param ulPort Endpoint port.
 * @param pxNetworkCredentials Pointer to Network credentials.
 * @param pxNetworkContext Point to Network context created.
 * @return uint32_t The status of the final connection attempt.
 */
static uint32_t prvConnectToServerWithBackoffRetries( const char * pcHostName,
                                                      uint32_t ulPort,
                                                      NetworkCredentials_t * pxNetworkCredentials,
                                                      NetworkContext_t * pxNetworkContext );
/*-----------------------------------------------------------*/

/**
 * @brief Static buffer used to hold MQTT messages being sent and received.
 */
static uint8_t ucMQTTMessageBuffer[ democonfigNETWORK_BUFFER_SIZE ];

/**
 * @brief Internal function for handling Command requests.
 *
 * @remark This function is required for the interface with samples to work properly.
 */
static void prvHandleCommand( AzureIoTHubClientCommandRequest_t * pxMessage,
                              void * pvContext )
{
    AzureIoTHubClient_t * pxHandle = ( AzureIoTHubClient_t * ) pvContext;
    uint32_t ulResponseStatus = 0;
    AzureIoTResult_t xResult;

    uint32_t ulCommandResponsePayloadLength = ulHandleCommand( pxMessage,
                                                               &ulResponseStatus,
                                                               ucCommandResponsePayloadBuffer,
                                                               sizeof( ucCommandResponsePayloadBuffer ) );

    if( ( xResult = AzureIoTHubClient_SendCommandResponse( pxHandle, pxMessage, ulResponseStatus,
                                                           ucCommandResponsePayloadBuffer,
                                                           ulCommandResponsePayloadLength ) ) != eAzureIoTSuccess )
    {
        LogError( ( "Error sending command response: result 0x%08x", xResult ) );
    }
    else
    {
        LogInfo( ( "Successfully sent command response %d", ulResponseStatus ) );
    }
}


static void prvDispatchPropertiesUpdate( AzureIoTHubClientPropertiesResponse_t * pxMessage )
{
    vHandleWritableProperties( pxMessage,
                               ucReportedPropertiesUpdate,
                               sizeof( ucReportedPropertiesUpdate ),
                               &ulReportedPropertiesUpdateLength );

    if( ulReportedPropertiesUpdateLength == 0 )
    {
        LogError( ( "Failed to send response to writable properties update, length of response is zero." ) );
    }
    else
    {
        AzureIoTResult_t xResult = AzureIoTHubClient_SendPropertiesReported( &xAzureIoTHubClient,
                                                                             ucReportedPropertiesUpdate,
                                                                             ulReportedPropertiesUpdateLength,
                                                                             NULL );
        configASSERT( xResult == eAzureIoTSuccess );
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Private property message callback handler.
 *        This handler dispatches the calls to the functions defined in
 *        sample_azure_iot_pnp_data_if.h
 */
static void prvHandleProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                 void * pvContext )
{
    ( void ) pvContext;

    LogDebug( ( "Property document payload : %.*s \r\n",
                pxMessage->ulPayloadLength,
                ( const char * ) pxMessage->pvMessagePayload ) );

    switch( pxMessage->xMessageType )
    {
        case eAzureIoTHubPropertiesRequestedMessage:
            LogDebug( ( "Device property document GET received" ) );
            prvDispatchPropertiesUpdate( pxMessage );
            break;

        case eAzureIoTHubPropertiesWritablePropertyMessage:
            LogDebug( ( "Device writeable property received" ) );
            prvDispatchPropertiesUpdate( pxMessage );
            break;

        case eAzureIoTHubPropertiesReportedResponseMessage:
            LogDebug( ( "Device reported property response received" ) );
            break;

        default:
            LogError( ( "Unknown property message: 0x%08x", pxMessage->xMessageType ) );
            configASSERT( false );
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Setup transport credentials.
 */
static uint32_t prvSetupNetworkCredentials( NetworkCredentials_t * pxNetworkCredentials )
{
    pxNetworkCredentials->xDisableSni = pdFALSE;
    /* Set the credentials for establishing a TLS connection. */
    pxNetworkCredentials->pucRootCa = ( const unsigned char * ) democonfigROOT_CA_PEM;
    pxNetworkCredentials->xRootCaSize = sizeof( democonfigROOT_CA_PEM );
    #ifdef democonfigCLIENT_CERTIFICATE_PEM
        pxNetworkCredentials->pucClientCert = ( const unsigned char * ) democonfigCLIENT_CERTIFICATE_PEM;
        pxNetworkCredentials->xClientCertSize = sizeof( democonfigCLIENT_CERTIFICATE_PEM );
        pxNetworkCredentials->pucPrivateKey = ( const unsigned char * ) democonfigCLIENT_PRIVATE_KEY_PEM;
        pxNetworkCredentials->xPrivateKeySize = sizeof( democonfigCLIENT_PRIVATE_KEY_PEM );
    #endif

    return 0;
}


/*-----------------------------------------------------------*/

char ucManifestBuffer[ 2500 ];
char * ucManifest = "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0=.eyJzaGEyNTYiOiJzaDRIcjZSNGxXNEtVRGdDNkJEQUo0dTNZMys1Z0srOXgxQVFSNnBTWEpRPSJ9.dSQKe7CPdj44VUSLlghLy-kSE5gsWEjPTLwdyGsgQTrxfk5fn-AJPe7VR5hVscfClPkI3EVpEbMN8rNDwDQupNnl0fp1phPr6kBj_j30UfRIvvd4lcoVYUABw-s8GhmuxShXrTPJWUSTGlTPlN1PdTvxIaRH2hoaMc6G2mZwoEQyjkAw_jqxXhYqYNXBl94AfcSTxqdGzHuEMfan7utaZgkS5HiGK_OFVlWXYFr3FpxoCcFmQDh-ANZJil25K4lngBuYTE5PgGguAD_ZvDBsyzw99ParYbgYXLk0wGagwPGhu2FANLmQuCsgYtXKt5lFs_hxJGEUTSslod6oXaw0utQlxRWgwMMIKTWAspzAm1XBsbpzB3FCF-97Qg5BzVexsRY3gtmKKp90eM0y5O_PXd7lAooPHf11BfQXJ2TCqcU9SdkPpQ4zZZILy5b2VP8vIgRL-VFwxzdyZpT3639t1oq1fnEtCBi91vLcXPMGq68iI7TrZH3slvTK3lSLlQO7";
char ucBase64DecodedHeader[ 1400 ];
char ucBase64DecodedPayload[ 60 ];
char ucBase64DecodedSignature[ 400 ];

char ucBase64DecodedJWKHeader[ 48 ];
char ucBase64DecodedJWKPayload[ 700 ];
char ucBase64EncodedJWKPayloadCopyWithEquals[ 700 ];
char ucBase64DecodedJWKSignature[ 500 ];

char ucCalculatationBuffer[512];

AzureIoTResult_t AzureSplitJWS( char * pucJWS,
                                uint32_t ulJWSLength,
                                char ** ppucHeader,
                                uint32_t * pulHeaderLength,
                                char ** ppucPayload,
                                uint32_t * pulPayloadLength,
                                char ** ppucSignature,
                                uint32_t * pulSignatureLength )
{
    char * pucFirstDot;
    char * pucSecondDot;
    uint32_t ulDotCount = 0;
    uint32_t ulIndex = 0;

    *ppucHeader = pucJWS;

    while( ulIndex < ulJWSLength )
    {
        if( *pucJWS == '.' )
        {
            ulDotCount++;

            if( ulDotCount == 1 )
            {
                pucFirstDot = pucJWS;
            }
            else if( ulDotCount == 2 )
            {
                pucSecondDot = pucJWS;
            }
            else if( ulDotCount > 2 )
            {
                return eAzureIoTErrorFailed;
            }
        }

        pucJWS++;
        ulIndex++;
    }

    if( ( ulDotCount != 2 ) || ( pucSecondDot >= ( *ppucHeader + ulJWSLength - 1 ) ) )
    {
        return eAzureIoTErrorFailed;
    }

    *pulHeaderLength = pucFirstDot - *ppucHeader;
    *ppucPayload = pucFirstDot + 1;
    *pulPayloadLength = pucSecondDot - *ppucPayload;
    *ppucSignature = pucSecondDot + 1;
    *pulSignatureLength = *ppucHeader + ulJWSLength - *ppucSignature;
}

AzureIoTResult_t AzureIoTSwapURLEncoding( char * pucSignature,
                                          uint32_t ulSignatureLength )
{
    uint32_t ulIndex = 0;

    char * hold = pucSignature;

    while( ulIndex < ulSignatureLength )
    {
        if( *pucSignature == '-' )
        {
            *pucSignature = '+';
        }
        else if( *pucSignature == '_' )
        {
            *pucSignature = '/';
        }

        pucSignature++;
        ulIndex++;
    }
}

#define azureiotRSA3072_SIZE 384
#define azureiotSHA256_SIZE 32

AzureIoTResult_t AzureIoT_SHA256Calculate( char * metadata,
                                       uint32_t metadataLength,
                                       char * input,
                                       uint32_t inputLength,
                                       char * output,
                                       uint32_t outputLength )
{
    return eAzureIoTSuccess;
}

AzureIoTResult_t AzureIoT_RS256Verify( char * input,
                                       uint32_t inputLength,
                                       char * signature,
                                       uint32_t signatureLength,
                                       char * n,
                                       uint32_t nLength,
                                       char * e,
                                       uint32_t eLength,
                                       char * buffer,
                                       uint32_t bufferLength )
{
  AzureIoTResult_t xResult;

  char *shaBuffer = buffer + azureiotRSA3072_SIZE;
  char* metadata;
  uint32_t metadataLength;
  xResult = AzureIoT_SHA256Calculate(metadata, metadataLength,
                                      input, inputLength,
                                      shaBuffer, azureiotSHA256_SIZE);

  return xResult;
}

void jws( void )
{
    char * pucHeader;
    char * pucPayload;
    char * pucSignature;
    uint32_t ulHeaderLength;
    uint32_t ulPayloadLength;
    uint32_t ulSignatureLength;
    AzureIoTJSONReader_t xJSONReader;

    memcpy( ucManifestBuffer, ucManifest, strlen( ucManifest ) );

    /*------------------- Parse and Decode the Manifest Sig ------------------------*/

    AzureIoTResult_t xResult = AzureSplitJWS( ucManifestBuffer, strlen( ucManifestBuffer ),
                                              &pucHeader, &ulHeaderLength,
                                              &pucPayload, &ulPayloadLength,
                                              &pucSignature, &ulSignatureLength );
    xResult = AzureIoTSwapURLEncoding( pucSignature, ulSignatureLength );

    printf( "---JWS Part One---\n" );
    int32_t outDecodedSizeOne;
    az_span decodedSpanHeader = az_span_create( ( ucBase64DecodedHeader ), sizeof( ucBase64DecodedHeader ) );
    az_span encodedHeaderSpan = az_span_create( ( uint8_t * ) pucHeader, ( uint32_t ) ulHeaderLength );
    az_result xCoreResult = az_base64_decode( decodedSpanHeader, encodedHeaderSpan, &outDecodedSizeOne );
    printf( "Core Return: 0x%x\n", xCoreResult );
    printf( "Out Decoded Size: %i\n", outDecodedSizeOne );
    printf( "%.*s\n\n", ( int ) outDecodedSizeOne, ucBase64DecodedHeader );

    printf( "---JWS Part Two---\n" );
    int32_t outDecodedSizeTwo;
    az_span decodedSpanPayload = az_span_create( ( ucBase64DecodedPayload ), sizeof( ucBase64DecodedPayload ) );
    az_span encodedPayloadSpan = az_span_create( ( uint8_t * ) pucPayload, ( uint32_t ) ulPayloadLength );
    xCoreResult = az_base64_decode( decodedSpanPayload, encodedPayloadSpan, &outDecodedSizeTwo );
    printf( "Core Return: 0x%x\n", xCoreResult );
    printf( "Out Decoded Size: %i\n", outDecodedSizeTwo );
    printf( "%.*s\n\n", ( int ) outDecodedSizeTwo, ucBase64DecodedPayload );

    printf( "---JWS Part Three---\n" );
    int32_t outDecodedSizeThree;
    az_span decodedSpanSignature = az_span_create( ( ucBase64DecodedSignature ), sizeof( ucBase64DecodedSignature ) );
    az_span encodedSignatureSpan = az_span_create( ( uint8_t * ) pucSignature, ( uint32_t ) ulSignatureLength );
    xCoreResult = az_base64_decode( decodedSpanSignature, encodedSignatureSpan, &outDecodedSizeThree );
    printf( "Core Return: 0x%x\n", xCoreResult );
    printf( "Out Decoded Size: %i\n", outDecodedSizeThree );
    printf( "%.*s\n\n", ( int ) outDecodedSizeThree, ucBase64DecodedSignature );


    /*------------------- Parse JSK JSON Payload ------------------------*/

    /* The "sjwk" is the signed signing public key */
    /* I believe as opposed to having a chain of trust for a public key, this is taking a known key */
    /* (baked into the device) and signing the key which was used to sign the manifest. */
    AzureIoTJSONReader_Init( &xJSONReader, ucBase64DecodedHeader, outDecodedSizeOne );
    xResult = AzureIoTJSONReader_NextToken( &xJSONReader );

    while( xResult == eAzureIoTSuccess )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "sjwk", strlen( "sjwk" ) ) )
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            break;
        }
        else
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            xResult = AzureIoTJSONReader_SkipChildren( &xJSONReader );
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
    }

    az_span xJWKManifestSpan = xJSONReader._internal.xCoreReader.token.slice;

    char * pucJWKManifest = az_span_ptr( xJWKManifestSpan );
    uint32_t ulJWKManifestLength = az_span_size( xJWKManifestSpan );

    /*------------------- Base64 Decode the JWK Payload ------------------------*/

    char * pucJWKHeader;
    char * pucJWKPayload;
    char * pucJWKSignature;
    uint32_t ulJWKHeaderLength;
    uint32_t ulJWKPayloadLength;
    uint32_t ulJWKSignatureLength;

    xResult = AzureSplitJWS( pucJWKManifest, ulJWKManifestLength,
                             &pucJWKHeader, &ulJWKHeaderLength,
                             &pucJWKPayload, &ulJWKPayloadLength,
                             &pucJWKSignature, &ulJWKSignatureLength );
    xResult = AzureIoTSwapURLEncoding( pucJWKSignature, ulJWKSignatureLength );

    printf( "---JWK Part One---\n" );
    int32_t outDecodedJWKSizeOne;
    az_span decodedJWKSpan = az_span_create( ( ucBase64DecodedJWKHeader ), sizeof( ucBase64DecodedJWKHeader ) );
    az_span encodedJWKHeaderSpan = az_span_create( ( uint8_t * ) pucJWKHeader, ( uint32_t ) ulJWKHeaderLength );
    xCoreResult = az_base64_decode( decodedJWKSpan, encodedJWKHeaderSpan, &outDecodedJWKSizeOne );
    printf( "Core Return: 0x%x\n", xCoreResult );
    printf( "Out Decoded Size: %i\n", outDecodedJWKSizeOne );
    printf( "%.*s\n\n", ( int ) outDecodedJWKSizeOne, ucBase64DecodedJWKHeader );

    printf( "---JWK Part Two---\n" );
    /* Have to hack in the padded characters */
    memcpy( ucBase64EncodedJWKPayloadCopyWithEquals, pucJWKPayload, ulJWKPayloadLength );
    ucBase64EncodedJWKPayloadCopyWithEquals[ ulJWKPayloadLength ] = '=';
    ucBase64EncodedJWKPayloadCopyWithEquals[ ulJWKPayloadLength + 1 ] = '=';
    ulJWKPayloadLength = ulJWKPayloadLength + 2;

    int32_t outDecodedJWKSizeTwo;
    az_span decodedJWKSpanTwo = az_span_create( ( ucBase64DecodedJWKPayload ), sizeof( ucBase64DecodedJWKPayload ) );
    az_span encodedJWKPayloadSpan = az_span_create( ( uint8_t * ) ucBase64EncodedJWKPayloadCopyWithEquals, ( uint32_t ) ulJWKPayloadLength );
    xCoreResult = az_base64_decode( decodedJWKSpanTwo, encodedJWKPayloadSpan, &outDecodedJWKSizeTwo );
    printf( "Core Return: 0x%x\n", xCoreResult );
    printf( "Out Decoded Size: %i\n", outDecodedJWKSizeTwo );
    printf( "%.*s\n\n", ( int ) outDecodedJWKSizeTwo, ucBase64DecodedJWKPayload );

    printf( "---JWK Part Three---\n" );
    int32_t outDecodedJWKSizeThree;
    az_span decodedJWKSpanThree = az_span_create( ( ucBase64DecodedJWKSignature ), sizeof( ucBase64DecodedJWKSignature ) );
    az_span encodedJWKSignatureSpan = az_span_create( ( uint8_t * ) pucJWKSignature, ( uint32_t ) ulJWKSignatureLength );
    xCoreResult = az_base64_decode( decodedJWKSpanThree, encodedJWKSignatureSpan, &outDecodedJWKSizeThree );
    printf( "Core Return: 0x%x\n", xCoreResult );
    printf( "Out Decoded Size: %i\n", outDecodedJWKSizeThree );
    printf( "%.*s\n\n", ( int ) outDecodedJWKSizeThree, ucBase64DecodedJWKSignature );

    /*------------------- Parse necessary pieces for the verification ------------------------*/
    az_span nSpan;
    az_span eSpan;
    az_span RS256Span;
    AzureIoTJSONReader_Init( &xJSONReader, ucBase64DecodedJWKPayload, outDecodedJWKSizeTwo );

    while( xResult == eAzureIoTSuccess )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "n", strlen( "n" ) ) )
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            nSpan = xJSONReader._internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            break;
        }
        else if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "e", strlen( "e" ) ) )
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            eSpan = xJSONReader._internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            break;
        }
        else if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "RS256", strlen( "RS256" ) ) )
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            RS256Span = xJSONReader._internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            break;
        }
        else
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            xResult = AzureIoTJSONReader_SkipChildren( &xJSONReader );
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
    }

    /*------------------- Verify the manifest ------------------------*/
    xResult = AzureIoT_RS256Verify(pucJWKHeader, ulJWKHeaderLength + ulJWKPayloadLength + 1,
                          ucBase64DecodedJWKSignature, outDecodedJWKSizeThree,
                          az_span_ptr(nSpan), az_span_size(nSpan),
                          az_span_ptr(eSpan), az_span_size(eSpan),
                          ucCalculatationBuffer, sizeof(ucCalculatationBuffer));

    /*------------------- Done (Loop) ------------------------*/
    while( 1 )
    {
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Azure IoT demo task that gets started in the platform specific project.
 *  In this demo task, middleware API's are used to connect to Azure IoT Hub and
 *  function to adhere to the Plug and Play device convention.
 */
static void prvAzureDemoTask( void * pvParameters )
{
    uint32_t ulScratchBufferLength = 0U;
    NetworkCredentials_t xNetworkCredentials = { 0 };
    AzureIoTTransportInterface_t xTransport;
    NetworkContext_t xNetworkContext = { 0 };
    TlsTransportParams_t xTlsTransportParams = { 0 };
    AzureIoTResult_t xResult;
    uint32_t ulStatus;
    AzureIoTHubClientOptions_t xHubOptions = { 0 };
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

    jws();

    /* Initialize Azure IoT Middleware.  */
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
        /* Attempt to establish TLS session with IoT Hub. If connection fails,
         * retry after a timeout. Timeout value will be exponentially increased
         * until  the maximum number of attempts are reached or the maximum timeout
         * value is reached. The function returns a failure status if the TCP
         * connection cannot be established to the IoT Hub after the configured
         * number of attempts. */
        ulStatus = prvConnectToServerWithBackoffRetries( ( const char * ) pucIotHubHostname,
                                                         democonfigIOTHUB_PORT,
                                                         &xNetworkCredentials, &xNetworkContext );
        configASSERT( ulStatus == 0 );

        /* Fill in Transport Interface send and receive function pointers. */
        xTransport.pxNetworkContext = &xNetworkContext;
        xTransport.xSend = TLS_Socket_Send;
        xTransport.xRecv = TLS_Socket_Recv;

        /* Init IoT Hub option */
        xResult = AzureIoTHubClient_OptionsInit( &xHubOptions );
        configASSERT( xResult == eAzureIoTSuccess );

        xHubOptions.pucModuleID = ( const uint8_t * ) democonfigMODULE_ID;
        xHubOptions.ulModuleIDLength = sizeof( democonfigMODULE_ID ) - 1;
        xHubOptions.pucModelID = ( const uint8_t * ) sampleazureiotMODEL_ID;
        xHubOptions.ulModelIDLength = sizeof( sampleazureiotMODEL_ID ) - 1;

        #ifdef democonfigPNP_COMPONENTS_LIST_LENGTH
            #if democonfigPNP_COMPONENTS_LIST_LENGTH > 0
                xHubOptions.pxComponentList = democonfigPNP_COMPONENTS_LIST;
                xHubOptions.ulComponentListLength = democonfigPNP_COMPONENTS_LIST_LENGTH;
            #endif /* > 0 */
        #endif /* democonfigPNP_COMPONENTS_LIST_LENGTH */

        xResult = AzureIoTHubClient_Init( &xAzureIoTHubClient,
                                          pucIotHubHostname, pulIothubHostnameLength,
                                          pucIotHubDeviceId, pulIothubDeviceIdLength,
                                          &xHubOptions,
                                          ucMQTTMessageBuffer, sizeof( ucMQTTMessageBuffer ),
                                          ullGetUnixTime,
                                          &xTransport );
        configASSERT( xResult == eAzureIoTSuccess );

        #ifdef democonfigDEVICE_SYMMETRIC_KEY
            xResult = AzureIoTHubClient_SetSymmetricKey( &xAzureIoTHubClient,
                                                         ( const uint8_t * ) democonfigDEVICE_SYMMETRIC_KEY,
                                                         sizeof( democonfigDEVICE_SYMMETRIC_KEY ) - 1,
                                                         Crypto_HMAC );
            configASSERT( xResult == eAzureIoTSuccess );
        #endif /* democonfigDEVICE_SYMMETRIC_KEY */

        /* Sends an MQTT Connect packet over the already established TLS connection,
         * and waits for connection acknowledgment (CONNACK) packet. */
        LogInfo( ( "Creating an MQTT connection to %s.\r\n", pucIotHubHostname ) );

        xResult = AzureIoTHubClient_Connect( &xAzureIoTHubClient,
                                             false, &xSessionPresent,
                                             sampleazureiotCONNACK_RECV_TIMEOUT_MS );
        configASSERT( xResult == eAzureIoTSuccess );

        xResult = AzureIoTHubClient_SubscribeCommand( &xAzureIoTHubClient, prvHandleCommand,
                                                      &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
        configASSERT( xResult == eAzureIoTSuccess );

        xResult = AzureIoTHubClient_SubscribeProperties( &xAzureIoTHubClient, prvHandleProperties,
                                                         &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT );
        configASSERT( xResult == eAzureIoTSuccess );

        /* Get property document after initial connection */
        xResult = AzureIoTHubClient_RequestPropertiesAsync( &xAzureIoTHubClient );
        configASSERT( xResult == eAzureIoTSuccess );

        /* Publish messages with QoS1, send and process Keep alive messages. */
        for( ; ; )
        {
            /* Hook for sending Telemetry */
            if( ( ulCreateTelemetry( ucScratchBuffer, sizeof( ucScratchBuffer ), &ulScratchBufferLength ) == 0 ) &&
                ( ulScratchBufferLength > 0 ) )
            {
                xResult = AzureIoTHubClient_SendTelemetry( &xAzureIoTHubClient,
                                                           ucScratchBuffer, ulScratchBufferLength,
                                                           NULL, eAzureIoTHubMessageQoS1, NULL );
                configASSERT( xResult == eAzureIoTSuccess );
            }

            /* Hook for sending update to reported properties */
            ulReportedPropertiesUpdateLength = ulCreateReportedPropertiesUpdate( ucReportedPropertiesUpdate, sizeof( ucReportedPropertiesUpdate ) );

            if( ulReportedPropertiesUpdateLength > 0 )
            {
                xResult = AzureIoTHubClient_SendPropertiesReported( &xAzureIoTHubClient, ucReportedPropertiesUpdate, ulReportedPropertiesUpdateLength, NULL );
                configASSERT( xResult == eAzureIoTSuccess );
            }

            LogInfo( ( "Attempt to receive publish message from IoT Hub.\r\n" ) );
            xResult = AzureIoTHubClient_ProcessLoop( &xAzureIoTHubClient,
                                                     sampleazureiotPROCESS_LOOP_TIMEOUT_MS );
            configASSERT( xResult == eAzureIoTSuccess );

            /* Leave Connection Idle for some time. */
            LogInfo( ( "Keeping Connection Idle...\r\n\r\n" ) );
            vTaskDelay( sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS );
        }

        xResult = AzureIoTHubClient_UnsubscribeProperties( &xAzureIoTHubClient );
        configASSERT( xResult == eAzureIoTSuccess );

        xResult = AzureIoTHubClient_UnsubscribeCommand( &xAzureIoTHubClient );
        configASSERT( xResult == eAzureIoTSuccess );

        /* Send an MQTT Disconnect packet over the already connected TLS over
         * TCP connection. There is no corresponding response for the disconnect
         * packet. After sending disconnect, client must close the network
         * connection. */
        xResult = AzureIoTHubClient_Disconnect( &xAzureIoTHubClient );
        configASSERT( xResult == eAzureIoTSuccess );

        /* Close the network connection.  */
        TLS_Socket_Disconnect( &xNetworkContext );

        /* Wait for some time between two iterations to ensure that we do not
         * bombard the IoT Hub. */
        LogInfo( ( "Demo completed successfully.\r\n" ) );
        LogInfo( ( "Short delay before starting the next iteration.... \r\n\r\n" ) );
        vTaskDelay( sampleazureiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS );
    }
}
/*-----------------------------------------------------------*/

#ifdef democonfigENABLE_DPS_SAMPLE

/**
 * @brief Get IoT Hub endpoint and device Id info, when Provisioning service is used.
 *   This function will block for Provisioning service for result or return failure.
 */
    static uint32_t prvIoTHubInfoGet( NetworkCredentials_t * pXNetworkCredentials,
                                      uint8_t ** ppucIothubHostname,
                                      uint32_t * pulIothubHostnameLength,
                                      uint8_t ** ppucIothubDeviceId,
                                      uint32_t * pulIothubDeviceIdLength )
    {
        NetworkContext_t xNetworkContext = { 0 };
        TlsTransportParams_t xTlsTransportParams = { 0 };
        AzureIoTResult_t xResult;
        AzureIoTTransportInterface_t xTransport;
        uint32_t ucSamplepIothubHostnameLength = sizeof( ucSampleIotHubHostname );
        uint32_t ucSamplepIothubDeviceIdLength = sizeof( ucSampleIotHubDeviceId );
        uint32_t ulStatus;

        /* Set the pParams member of the network context with desired transport. */
        xNetworkContext.pParams = &xTlsTransportParams;

        ulStatus = prvConnectToServerWithBackoffRetries( democonfigENDPOINT, democonfigIOTHUB_PORT,
                                                         pXNetworkCredentials, &xNetworkContext );
        configASSERT( ulStatus == 0 );

        /* Fill in Transport Interface send and receive function pointers. */
        xTransport.pxNetworkContext = &xNetworkContext;
        xTransport.xSend = TLS_Socket_Send;
        xTransport.xRecv = TLS_Socket_Recv;

        xResult = AzureIoTProvisioningClient_Init( &xAzureIoTProvisioningClient,
                                                   ( const uint8_t * ) democonfigENDPOINT,
                                                   sizeof( democonfigENDPOINT ) - 1,
                                                   ( const uint8_t * ) democonfigID_SCOPE,
                                                   sizeof( democonfigID_SCOPE ) - 1,
                                                   ( const uint8_t * ) democonfigREGISTRATION_ID,
                                                   sizeof( democonfigREGISTRATION_ID ) - 1,
                                                   NULL, ucMQTTMessageBuffer, sizeof( ucMQTTMessageBuffer ),
                                                   ullGetUnixTime,
                                                   &xTransport );
        configASSERT( xResult == eAzureIoTSuccess );

        #ifdef democonfigDEVICE_SYMMETRIC_KEY
            xResult = AzureIoTProvisioningClient_SetSymmetricKey( &xAzureIoTProvisioningClient,
                                                                  ( const uint8_t * ) democonfigDEVICE_SYMMETRIC_KEY,
                                                                  sizeof( democonfigDEVICE_SYMMETRIC_KEY ) - 1,
                                                                  Crypto_HMAC );
            configASSERT( xResult == eAzureIoTSuccess );
        #endif /* democonfigDEVICE_SYMMETRIC_KEY */

        xResult = AzureIoTProvisioningClient_SetRegistrationPayload( &xAzureIoTProvisioningClient,
                                                                     ( const uint8_t * ) sampleazureiotPROVISIONING_PAYLOAD,
                                                                     sizeof( sampleazureiotPROVISIONING_PAYLOAD ) - 1 );
        configASSERT( xResult == eAzureIoTSuccess );

        do
        {
            xResult = AzureIoTProvisioningClient_Register( &xAzureIoTProvisioningClient,
                                                           sampleazureiotProvisioning_Registration_TIMEOUT_MS );
        } while( xResult == eAzureIoTErrorPending );

        if( xResult == eAzureIoTSuccess )
        {
            LogInfo( ( "Successfully acquired IoT Hub name and Device ID" ) );
        }
        else
        {
            LogInfo( ( "Error geting IoT Hub name and Device ID: 0x%08x", xResult ) );
        }

        configASSERT( xResult == eAzureIoTSuccess );

        xResult = AzureIoTProvisioningClient_GetDeviceAndHub( &xAzureIoTProvisioningClient,
                                                              ucSampleIotHubHostname, &ucSamplepIothubHostnameLength,
                                                              ucSampleIotHubDeviceId, &ucSamplepIothubDeviceIdLength );
        configASSERT( xResult == eAzureIoTSuccess );

        AzureIoTProvisioningClient_Deinit( &xAzureIoTProvisioningClient );

        /* Close the network connection.  */
        TLS_Socket_Disconnect( &xNetworkContext );

        *ppucIothubHostname = ucSampleIotHubHostname;
        *pulIothubHostnameLength = ucSamplepIothubHostnameLength;
        *ppucIothubDeviceId = ucSampleIotHubDeviceId;
        *pulIothubDeviceIdLength = ucSamplepIothubDeviceIdLength;

        return 0;
    }

#endif /* democonfigENABLE_DPS_SAMPLE */
/*-----------------------------------------------------------*/

/**
 * @brief Connect to server with backoff retries.
 */
static uint32_t prvConnectToServerWithBackoffRetries( const char * pcHostName,
                                                      uint32_t port,
                                                      NetworkCredentials_t * pxNetworkCredentials,
                                                      NetworkContext_t * pxNetworkContext )
{
    TlsTransportStatus_t xNetworkStatus;
    BackoffAlgorithmStatus_t xBackoffAlgStatus = BackoffAlgorithmSuccess;
    BackoffAlgorithmContext_t xReconnectParams;
    uint16_t usNextRetryBackOff = 0U;

    /* Initialize reconnect attempts and interval. */
    BackoffAlgorithm_InitializeParams( &xReconnectParams,
                                       sampleazureiotRETRY_BACKOFF_BASE_MS,
                                       sampleazureiotRETRY_MAX_BACKOFF_DELAY_MS,
                                       sampleazureiotRETRY_MAX_ATTEMPTS );

    /* Attempt to connect to IoT Hub. If connection fails, retry after
     * a timeout. Timeout value will exponentially increase till maximum
     * attempts are reached.
     */
    do
    {
        LogInfo( ( "Creating a TLS connection to %s:%u.\r\n", pcHostName, port ) );
        /* Attempt to create a mutually authenticated TLS connection. */
        xNetworkStatus = TLS_Socket_Connect( pxNetworkContext,
                                             pcHostName, port,
                                             pxNetworkCredentials,
                                             sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS,
                                             sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS );

        if( xNetworkStatus != eTLSTransportSuccess )
        {
            /* Generate a random number and calculate backoff value (in milliseconds) for
             * the next connection retry.
             * Note: It is recommended to seed the random number generator with a device-specific
             * entropy source so that possibility of multiple devices retrying failed network operations
             * at similar intervals can be avoided. */
            xBackoffAlgStatus = BackoffAlgorithm_GetNextBackoff( &xReconnectParams, configRAND32(), &usNextRetryBackOff );

            if( xBackoffAlgStatus == BackoffAlgorithmRetriesExhausted )
            {
                LogError( ( "Connection to the IoT Hub failed, all attempts exhausted." ) );
            }
            else if( xBackoffAlgStatus == BackoffAlgorithmSuccess )
            {
                LogWarn( ( "Connection to the IoT Hub failed [%d]. "
                           "Retrying connection with backoff and jitter [%d]ms.",
                           xNetworkStatus, usNextRetryBackOff ) );
                vTaskDelay( pdMS_TO_TICKS( usNextRetryBackOff ) );
            }
        }
    } while( ( xNetworkStatus != eTLSTransportSuccess ) && ( xBackoffAlgStatus == BackoffAlgorithmSuccess ) );

    return xNetworkStatus == eTLSTransportSuccess ? 0 : 1;
}
/*-----------------------------------------------------------*/

/*
 * @brief Create the task that demonstrates the AzureIoTHub demo
 */
void vStartDemoTask( void )
{
    /* This example uses a single application task, which in turn is used to
     * connect, subscribe, publish, unsubscribe and disconnect from the IoT Hub */
    xTaskCreate( prvAzureDemoTask,         /* Function that implements the task. */
                 "AzureDemoTask",          /* Text name for the task - only used for debugging. */
                 democonfigDEMO_STACKSIZE, /* Size of stack (in words, not bytes) to allocate for the task. */
                 NULL,                     /* Task parameter - not used in this case. */
                 tskIDLE_PRIORITY,         /* Task priority, must be between 0 and configMAX_PRIORITIES - 1. */
                 NULL );                   /* Used to pass out a handle to the created task - not used in this case. */
}
/*-----------------------------------------------------------*/
