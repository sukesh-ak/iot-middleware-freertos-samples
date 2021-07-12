/**
 * @file esp_crypto.c
 * @author tawalke
 * @brief Implements mbed TLS crypto platform functions for ESP32 using the ESP32 mBedTLS 
 * component and the ESP32 mBedTLS HMAC abstractions that provide options for ESP32 hardware 
 * acceleration, Secure Element or Software HMAC processing.
 */

#include "esp_crypto.h"

uint32_t Crypto_Init()
{
    uint32_t ret_value = 0;

    return ret_value;
}

uint32_t Crypto_HMAC( const uint8_t * pucKey, uint32_t ulKeyLength,
                      const uint8_t * pucData, uint32_t ulDataLength,
                      uint8_t * pucOutput, uint32_t ulOutputLength,
                      uint32_t * pulBytesCopied )
{
    uint32_t ret_value = 0;

    return ret_value;

}