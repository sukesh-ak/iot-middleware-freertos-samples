#ifndef _AZ_FREERTOS_DPSCLIENT_H_
#define _AZ_FREERTOS_DPSCLIENT_H_

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "esp_tls.h"

#include "az_esp_transport_wrapper.h"

/* Define buffer for IoTHub info.  */
static uint8_t ucSampleIotHubHostname[ 128 ];
static uint8_t ucSampleIotHubDeviceId[ 128 ];


static AzureIoTProvisioningClient_t xAzureIoTProvisioningClient;

/**
 * @brief Gets the IoTHub endpoint and deviceId from Provisioning service.
 *
 * @param[in] pXNetworkCredentials  Network credential used to connect to Provisioning service
 * @param[out] ppucIothubHostname  Pointer to uint8_t* IoTHub hostname return from Provisioning Service
 * @param[inout] pulIothubHostnameLength  Length of hostname
 * @param[out] ppucIothubDeviceId  Pointer to uint8_t* deviceId return from Provisioning Service
 * @param[inout] pulIothubDeviceIdLength  Length of deviceId
 */
    static uint32_t prvIoTHubInfoGet( NetworkCredentials_t * pXNetworkCredentials,
                                      uint8_t ** ppucIothubHostname,
                                      uint32_t * pulIothubHostnameLength,
                                      uint8_t ** ppucIothubDeviceId,
                                      uint32_t * pulIothubDeviceIdLength );

#endif
