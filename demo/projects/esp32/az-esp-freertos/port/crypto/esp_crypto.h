/* SPDX-License-Identifier: MIT */

/**
 * @file esp_crypto.h
 * @author tawalke
 * @brief Header file to implements mbed TLS crypto platform functions for ESP32 using the ESP32 
 * components (ex. mBedTLS, libSodium, etc.) for sha256 HMAC encryption. Keeping signatures simple to
 * ones used for crypto port from FreeRTOS that is used in Azure Middleware.
 */

#ifndef ESP_CRYPTO_H
#define ESP_CRYPTO_H


#include <stdint.h>

#define ESP_USE_HMAC    1 // MBedtls functions; 1: Use HMAC 0: Don't Use HMAC 

uint32_t Crypto_Init();

uint32_t Crypto_HMAC( const uint8_t * pucKey, uint32_t ulKeyLength,
                      const uint8_t * pucData, uint32_t ulDataLength,
                      uint8_t * pucOutput, uint32_t ulOutputLength,
                      uint32_t * pulBytesCopied ); 

#endif
