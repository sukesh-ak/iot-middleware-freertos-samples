#include <assert.h>

#include "azure_iot_mqtt.h"

/**
 * Maps ESP-MQTT errors to AzureIoTMQTT errors.
 **/

/**Mapping ESP Error Codes to Azure IoT. 
 * These are very different than CoreMQTT codes and Azure errors are related to this
 * Mapped as Azure errors closely as possible.
**/

static AzureIoTMQTTResult_t prvTranslateToAzureIoTTLSResult(esp_err_t xResult)
{
    //using same structure to be consistent (should discuss)
    AzureIoTMQTTResult_t xReturn; 

    switch ( xResult )
    {
        case ESP_ERR_ESP_TLS_CANNOT_RESOLVE_HOSTNAME : 
        {
            xReturn = eAzureIoTMQTTBadParameter;
        }
        break; 

        case ESP_ERR_ESP_TLS_CANNOT_CREATE_SOCKET  : 
        {
            xReturn = eAzureIoTMQTTBadResponse;
        }
        break;

        case ESP_ERR_ESP_TLS_UNSUPPORTED_PROTOCOL_FAMILY :
        {
            xReturn = eAzureIoTMQTTBadParameter;
        } 
        break; 

        case ESP_ERR_ESP_TLS_FAILED_CONNECT_TO_HOST : 
        {
            xReturn = eAzureIoTMQTTServerRefused;
        }
        break; 

        case ESP_ERR_ESP_TLS_SOCKET_SETOPT_FAILED :
        {
            xReturn = eAzureIoTMQTTBadParameter;
        }
        break; 

        case ESP_ERR_MBEDTLS_CERT_PARTLY_OK : 
        {
            xReturn = eAzureIoTMQTTNoDataAvailable; //not sure
        }
        break; 

        case ESP_ERR_MBEDTLS_CTR_DRBG_SEED_FAILED :  
        {
            xReturn = eAzureIoTMQTTBadParameter;
        }
        break; 

        case ESP_ERR_MBEDTLS_SSL_SET_HOSTNAME_FAILED :
        {
            xReturn = eAzureIoTMQTTBadParameter;
        }
        break; 

        case ESP_ERR_MBEDTLS_SSL_CONFIG_DEFAULTS_FAILED : 
        {
            xReturn = eAzureIoTMQTTBadParameter;
        }
        break; 

        case ESP_ERR_MBEDTLS_SSL_CONF_ALPN_PROTOCOLS_FAILED : 
        {
            xReturn = eAzureIoTMQTTFailed;
        }
        break; 

        case ESP_ERR_MBEDTLS_X509_CRT_PARSE_FAILED : 
        {
            xReturn = eAzureIoTMQTTBadResponse;
        }
        break; 

        case ESP_ERR_MBEDTLS_SSL_CONF_OWN_CERT_FAILED :
        {
            xReturn = eAzureIoTMQTTBadResponse;
        }
        break; 

        case ESP_ERR_MBEDTLS_SSL_SETUP_FAILED :
        {
            xReturn = eAzureIoTMQTTBadResponse;
        }
        break;

        case ESP_ERR_MBEDTLS_SSL_WRITE_FAILED : 
        {
            xReturn = eAzureIoTMQTTSendFailed;
        }
        break; 

        case ESP_ERR_MBEDTLS_PK_PARSE_KEY_FAILED :
        {
            xReturn = eAzureIoTMQTTBadParameter;
        }
        break; 

        case ESP_ERR_MBEDTLS_SSL_HANDSHAKE_FAILED :
        {
            xReturn = eAzureIoTMQTTNoDataAvailable
        }
        break; 

        case ESP_ERR_MBEDTLS_SSL_CONF_PSK_FAILED :
        {
            xReturn = eAzureIoTMQTTBadResponse;
        }
        break; 

        case ESP_ERR_ESP_TLS_CONNECTION_TIMEOUT :
        {
            xReturn = eAzureIoTMQTTKeepAliveTimeout; 
        } 
        break; 

        case ESP_ERR_ESP_TLS_SE_FAILED :
        {
            xReturn = eAzureIoTMQTTNoDataAvailable;
        }
        break;

    }

    return xReturn; 

    
}

static AzureIoTMQTTResult_t prvTranslateToAzureIoTMQTTResult(esp_mqtt_error_codes_t xResult )
{
    AzureIoTMQTTResult_t xReturn;
    esp_mqtt_connect_return_code_t x_mqtt_return_type;
    
    x_mqtt_return_type = xResult.error_type;

    switch ( x_mqtt_return_type )
    {
        case MQTT_ERROR_TYPE_NONE :
        {
            xReturn = eAzureIoTMQTTSuccess;
        }
        break; 

        case MQTT_ERROR_TYPE_ESP_TLS :
        {
            esp_err_t x_tls_error = xResult.esp_tls_last_esp_err;

            xReturn = prvTranslateToAzureIoTTLSResult(x_tls_error);
        }
        break;

        case MQTT_ERROR_TYPE_CONNECTION_REFUSED :
        {
            esp_mqtt_connect_return_code_t x_mqtt_connect_error = xResult.connect_return_code; 

            switch ( x_mqtt_connect_error )
            {
                case MQTT_CONNECTION_ACCEPTED : 
                {
                    xReturn = eAzureIoTMQTTSuccess;
                }
                break;

                case MQTT_CONNECTION_REFUSE_PROTOCOL :
                {
                    xReturn = eAzureIoTMQTTServerRefused;
                }
                break; 
                
                case MQTT_CONNECTION_REFUSE_ID_REJECTED :
                {
                    xReturn = eAzureIoTMQTTBadParameter;
                }
                break;

                case MQTT_CONNECTION_REFUSE_SERVER_UNAVAILABLE : 
                {
                    xReturn = eAzureIoTMQTTFailed;
                }
                break; 

                case MQTT_CONNECTION_REFUSE_BAD_USERNAME : 
                {
                    xReturn = eAzureIoTMQTTBadParameter;
                }
                break; 
                
                case MQTT_CONNECTION_REFUSE_NOT_AUTHORIZED : 
                {
                    xReturn = eAzureIoTMQTTServerRefused;
                }
                break; 
            }
        }
        break; 
    }

    return xReturn;
}

AzureIoTMQTTResult_t AzureIoTMQTT_Init( AzureIoTMQTTHandle_t xContext,
                                        const AzureIoTTransportInterface_t * pxTransportInterface,
                                        AzureIoTMQTTGetCurrentTimeFunc_t xGetTimeFunction,
                                        AzureIoTMQTTEventCallback_t xUserCallback,
                                        uint8_t * pucNetworkBuffer,
                                        size_t xNetworkBufferLength )
{
    MQTTFixedBuffer_t xBuffer = { pucNetworkBuffer, xNetworkBufferLength };
    MQTTStatus_t xResult;

    /* Check memory equivalence, but ordering is not guaranteed */
    assert( sizeof( AzureIoTMQTTConnectInfo_t ) == sizeof( MQTTConnectInfo_t ) );
    assert( sizeof( AzureIoTMQTTSubscribeInfo_t ) == sizeof( MQTTSubscribeInfo_t ) );
    assert( sizeof( AzureIoTMQTTPacketInfo_t ) == sizeof( MQTTPacketInfo_t ) );
    assert( sizeof( AzureIoTMQTTPublishInfo_t ) == sizeof( MQTTPublishInfo_t ) );
    assert( sizeof( AzureIoTMQTTResult_t ) == sizeof( MQTTStatus_t ) );
    assert( sizeof( AzureIoTTransportInterface_t ) == sizeof( TransportInterface_t ) );

    xResult = MQTT_Init( xContext,
                         ( const TransportInterface_t * ) pxTransportInterface,
                         ( MQTTGetCurrentTimeFunc_t ) xGetTimeFunction,
                         ( MQTTEventCallback_t ) xUserCallback,
                         &xBuffer );

    return prvTranslateToAzureIoTMQTTResult( xResult );
}

AzureIoTMQTTResult_t AzureIoTMQTT_Connect( AzureIoTMQTTHandle_t xContext,
                                           const AzureIoTMQTTConnectInfo_t * pxConnectInfo,
                                           const AzureIoTMQTTPublishInfo_t * pxWillInfo,
                                           uint32_t ulMilliseconds,
                                           bool * pxSessionPresent )
{
    MQTTStatus_t xResult;

    xResult =  MQTT_Connect( xContext,
                             ( const MQTTConnectInfo_t * ) pxConnectInfo,
                             ( const MQTTPublishInfo_t * ) pxWillInfo,
                             ulMilliseconds, pxSessionPresent );
    
    return prvTranslateToAzureIoTMQTTResult( xResult );
}

AzureIoTMQTTResult_t AzureIoTMQTT_Subscribe( AzureIoTMQTTHandle_t xContext,
                                             const AzureIoTMQTTSubscribeInfo_t * pxSubscriptionList,
                                             size_t xSubscriptionCount,
                                             uint16_t usPacketId )
{
    MQTTStatus_t xResult;

    xResult = MQTT_Subscribe( xContext, ( const MQTTSubscribeInfo_t * ) pxSubscriptionList,
                              xSubscriptionCount, usPacketId );

    return prvTranslateToAzureIoTMQTTResult( xResult );
}
