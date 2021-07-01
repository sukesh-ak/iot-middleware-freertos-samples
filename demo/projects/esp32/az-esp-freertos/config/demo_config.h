// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef DEMO_CONFIG_H
#define DEMO_CONFIG_H

/* FreeRTOS config include. */
#include "freertos/FreeRTOSConfig.h"

/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Include logging header files and define logging macros in the following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define the LIBRARY_LOG_NAME and LIBRARY_LOG_LEVEL macros depending on
 * the logging configuration for DEMO.
 * 3. Include the header file "logging_stack.h", if logging is enabled for DEMO.
 */

#include "logging_levels.h"

/* Logging configuration for the Demo. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME    "Azure_FreeRTOS_ESP32"
#endif

#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_INFO
#endif

/* Prototype for the function used to print to console on Windows simulator
 * of FreeRTOS.
 * The function prints to the console before the network is connected;
 * then a UDP port after the network has connected. */
extern void vLoggingPrintf( const char * pcFormatString,
                            ... );

/* Map the SdkLog macro to the logging function to enable logging
 * on Windows simulator. */
#ifndef SdkLog
    #define SdkLog( message )    vLoggingPrintf message
#endif

#include "logging_stack.h"
#include "platform.h"

/************ End of logging configuration ****************/

/**
 * @brief Enable Device Provisioning
 * 
 * @note To disable Device Provisioning undef this macro
 *
 */
#define democonfigENABLE_DPS_SAMPLE

#ifdef democonfigENABLE_DPS_SAMPLE

/**
 * @brief Provisioning service endpoint.
 *
 * @note https://docs.microsoft.com/en-us/azure/iot-dps/concepts-service#service-operations-endpoint
 * 
 */
#define democonfigENDPOINT                  "<YOUR DPS ENDPOINT HERE>"

/**
 * @brief Id scope of provisioning service.
 * 
 * @note https://docs.microsoft.com/en-us/azure/iot-dps/concepts-service#id-scope
 * 
 */
#define democonfigID_SCOPE                  "<YOUR ID SCOPE HERE>"

/**
 * @brief Registration Id of provisioning service
 * 
 * @warning If using X509 authentication, this MUST match the Common Name of the cert.
 *
 *  @note https://docs.microsoft.com/en-us/azure/iot-dps/concepts-service#registration-id
 */
#define democonfigREGISTRATION_ID           "<YOUR REGISTRATION ID HERE>"

#endif // democonfigENABLE_DPS_SAMPLE

/**
 * @brief IoTHub device Id.
 *
 */
#define democonfigDEVICE_ID                 CONFIG_MQTT_CLIENT_IDENTIFIER

/**
 * @brief IoTHub module Id.
 *
 */
#define democonfigMODULE_ID                 ""
/**
 * @brief IoTHub hostname.
 *
 */
#define democonfigHOSTNAME                  CONFIG_MQTT_BROKER_ENDPOINT

/**
 * @brief Device symmetric key
 *
 */
#define democonfigDEVICE_SYMMETRIC_KEY      CONFIG_IOTHUB_DEVICE_SYMMETRIC_KEY

/**
 * @brief Client's X509 Certificate.
 *
 */
// #define democonfigCLIENT_CERTIFICATE_PEM    "<YOUR DEVICE CERT HERE>"

/**
 * @brief Client's private key.
 * 
 */
// #define democonfigCLIENT_PRIVATE_KEY_PEM    "<YOUR DEVICE PRIVATE KEY HERE>"

/**
 * @brief Baltimore Trusted RooT CA.
 *
 */
#define democonfigROOT_CA_PEM               CONFIG_IOTHUB_ROOTCA_PEM

/**
 * @brief An option to disable Server Name Indication.
 *
 * @note When using a local Mosquitto server setup, SNI needs to be disabled
 * for an MQTT broker that only has an IP address but no hostname. However,
 * SNI should be enabled whenever possible.
 */
#define democonfigDISABLE_SNI              CONFIG_MQTT_OPTION_SNI //( pdFALSE )

/**
 * @brief The name of the operating system that the application is running on.
 * The current value is given as an example. Please update for your specific
 * operating system.
 */
#define democonfigOS_NAME                   "FreeRTOS"

/**
 * @brief The version of the operating system that the application is running
 * on. The current value is given as an example. Please update for your specific
 * operating system version.
 */
#define democonfigOS_VERSION                tskKERNEL_VERSION_NUMBER

/**
 * @brief Set the stack size of the main demo task.
 *
 * In the Windows port, this stack only holds a structure. The actual
 * stack is created by an operating system thread.
 */
#define democonfigDEMO_STACKSIZE            ( 2 * 1024U) //TODO: Change this to CONFIG val after test-TEW

/**
 * @brief Size of the network buffer for MQTT packets.
 */
#define democonfigNETWORK_BUFFER_SIZE       ( 5 * 1024U ) //TODO: Change this to CONFIG val after test-TEW

/**
 * @brief IoTHub endpoint port.
 */
#define democonfigMQTT_BROKER_PORT          CONFIG_MQTT_BROKER_PORT

#endif /* DEMO_CONFIG_H */
