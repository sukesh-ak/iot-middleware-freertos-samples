/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#ifndef AZ_ESP_TRANSPORT_WRAPPER_H
#define AZ_ESP_TRANSPORT_WRAPPER_H

#include "azure_iot_transport_interface.h"
#include "esp_tls.h"

struct NetworkContext
{
    esp_tls_t tcpSocketContext;
};

typedef struct NetworkContext NetworkContext_t;


static int prvConnectToServerWithBackoffRetries( const char * pcHostName,
                                                 uint32_t ulPort,
                                                 esp_tls_cfg_t * pxNetworkCredentials,
                                                 NetworkContext_t * pxNetworkContext );

static uint32_t prvSetupNetworkCredentials( esp_tls_cfg_t * pxNetworkCredentials );