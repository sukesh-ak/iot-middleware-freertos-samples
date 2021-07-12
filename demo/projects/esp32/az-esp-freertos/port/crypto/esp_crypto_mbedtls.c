/**
 * @file esp_crypto_mbedtls.c
 * @author tawalke
 * @brief Implements mbed TLS crypto platform functions for ESP32 using the ESP32 mBedTLS 
 * component. This uses the MBED HMAC functions directly vs. the ESP32 mBedTLS HMAC abstractions
 * options for 256sha encryption shown in esp_crypto.c part of this library. 
 */

#include "esp_crypto.h"
#include "mbedtls/md.h"

#define ESP_HMAC_MD_FAIL              1;
#define ESP_HMAC_MD_SUCCESS           0;

mbedtls_md_type_t esp_hmac_md_type = MBEDTLS_MD_SHA256; //Config to expand SHA offering?
mbedtls_md_context_t esp_md_ctx = NULL;


uint32_t esp_md_setup_init( mbedtls_md_context_t *esp_ctx, mbedtls_md_type_t esp_md_type)
{
    int md_result = 0;
    uint32_t retval;

    mbedtls_md_init(&esp_ctx);

    md_result = mbedtls_md_setup(&esp_ctx, 
                                 mbedtls_md_info_from_type(esp_md_type), 
                                 ESP_USE_HMAC); 
    
    retval = (md_result != 0)   ? (uint32_t)ESP_HMAC_MD_FAIL 
                                : (uint32_t)ESP_HMAC_MD_SUCCESS;

    // TODO: Add ESP logging for result & return

    return retval;
}

uint32_t esp_md_hmac_calculate( mbedtls_md_context_t * esp_ctx, uint8_t * esp_md_key,  
                                uint32_t esp_md_keylen, uint8_t * esp_data_payload, 
                                uint32_t esp_data_payloadlen, uint8_t * esp_hmac_result)
{
    
    int md_result = 0;
    uint32_t retval;

    //
    md_result =  mbedtls_md_hmac_starts(&esp_ctx, esp_md_key, esp_md_keylen);
    md_result += mbedtls_md_hmac_update(&esp_ctx, esp_data_payload, esp_data_payloadlen);
    md_result += mbedtls_md_hmac_finish(&esp_ctx, esp_hmac_result);
     
    retval = (md_result != 0)   ? (uint32_t)ESP_HMAC_MD_FAIL 
                                : (uint32_t)ESP_HMAC_MD_SUCCESS; 

    // TODO: Add ESP logging for result & return

    return retval;
}

void esp_md_cleanup(mbedtls_md_context_t * esp_ctx)
{
    mbedtls_md_free(&esp_ctx);
     // TODO: Add ESP logging for result & return
}


// Seems Thread safety is covered in ESP32 this way it seems. 
// Keeping in case but not needed
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
    uint32_t ret_value == ESP_HMAC_MD_SUCCESS;
    int md_result = 0;

    esp_md_setup_init(&esp_md_ctx);
    esp_md_hmac_calculate(&esp_md_ctx, pucKey, ulKeyLength, 
                          pucData, ulDataLength, pucOutput);
    esp_md_cleanup(&esp_md_ctx);

    // TODO: Add error checking & ESP LOG
    return ret_value;

}