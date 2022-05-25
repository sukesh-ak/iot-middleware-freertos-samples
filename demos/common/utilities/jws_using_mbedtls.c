/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "azure/az_core.h"
#include "azure/az_iot.h"

#include "azure_iot_result.h"
#include "azure_iot_json_reader.h"
#include "azure_iot_adu_client.h"

#include "mbedtls/base64.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/cipher.h"



#define azureiotRSA3072_SIZE    384
#define azureiotSHA256_SIZE     32
#define azureiotMODULUS_SIZE    384

char ucManifestBuffer[ 2500 ];
char * ucEscapedManifest = "{\"manifestVersion\":\"4\",\"updateId\":{\"provider\":\"ESPRESSIF\",\"name\":\"ESP32-Azure-IoT-Kit\",\"version\":\"1.1\"},\"compatibility\":[{\"deviceManufacturer\":\"ESPRESSIF\",\"deviceModel\":\"ESP32-Azure-IoT-Kit\"}],\"instructions\":{\"steps\":[{\"handler\":\"microsoft/swupdate:1\",\"files\":[\"f9fec76f10aede60e\"],\"handlerProperties\":{\"installedCriteria\":\"1.0\"}}]},\"files\":{\"f9fec76f10aede60e\":{\"fileName\":\"azure_iot_freertos_esp32-v1.1.bin\",\"sizeInBytes\":861520,\"hashes\":{\"sha256\":\"BwsqbyduNatbrmHaLauoxeC1EY4J8Dv7mE76RxUfUAk=\"}}},\"createdDateTime\":\"2022-04-19T15:52:45.8497679Z\"}";
char * ucManifestSignature = "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0=.eyJzaGEyNTYiOiJJVHB3ZWxTTjJRWHBBaDFDNVoxWjVDNmV3VzNHK2kvM1VKcm1kZ1lSaG1VPSJ9.jmh3bEm-pfjzlxJfylexPX0fUqMeCiyP5uvFTd0QdAIk9cMIxv-8_SfzCTrhT-SvTf8XFTLkoFbhgsa0y5nTLxSm8Y2fR3WvkGIZGGywV89zQ-fEGnmM4lsiunlXI6hrVv3uQQeKhjcRWAgwpcwXE4xlP2cejPV9Auxy8rcRnNrf5-3Y21M1QBmCgkDj4Kv9xABo9U5w90I5XEu4hzbWbLAr-KLCiokck4rze6zLKjistAge8VtC318yBiEN2np_GQAXvt_IHkSkS6VTxryCX7hA5TdNQiugQvoXzTPVeRa0WmfAjk1FQZq_JuWJbEazPrhzQtKDhikO9aD-O9Ju6SCTsDVdmUIv0Hn_aq2a3l1rvcajhfpsPGdy3Lw605AGs1Fctu7W3jK5IFcoOstNvW905ywOV_NP5xlcm3vn17kGjgXAgeSaWo1NDT11ghPwZy82M5mKEof9gRy_edu99TSB00MRo3TX-vE80FIrjIrBAFToTDjRadE04JFfaiOX";
char * ucManifestShort = "eyJhbGciOiJSUzI1NiIsInNqd2siOiJleUpoYkdjaU9pSlNVekkxTmlJc0ltdHBaQ0k2SWtGRVZTNHlNREEzTURJdVVpSjkuZXlKcmRIa2lPaUpTVTBFaUxDSnVJam9pYkV4bWMwdHZPRmwwWW1Oak1sRXpUalV3VlhSTVNXWlhVVXhXVTBGRlltTm9LMFl2WTJVM1V6Rlpja3BvV0U5VGNucFRaa051VEhCVmFYRlFWSGMwZWxndmRHbEJja0ZGZFhrM1JFRmxWVzVGU0VWamVEZE9hM2QzZVRVdk9IcExaV3AyWTBWWWNFRktMMlV6UWt0SE5FVTBiMjVtU0ZGRmNFOXplSGRQUzBWbFJ6QkhkamwzVjB3emVsUmpUblprUzFoUFJGaEdNMVZRWlVveGIwZGlVRkZ0Y3pKNmJVTktlRUppZEZOSldVbDBiWFpwWTNneVpXdGtWbnBYUm5jdmRrdFVUblZMYXpob2NVczNTRkptYWs5VlMzVkxXSGxqSzNsSVVVa3dZVVpDY2pKNmEyc3plR2d4ZEVWUFN6azRWMHBtZUdKamFsQnpSRTgyWjNwWmVtdFlla05OZW1Fd1R6QkhhV0pDWjB4QlZGUTVUV1k0V1ZCd1dVY3lhblpQWVVSVmIwTlJiakpWWTFWU1RtUnNPR2hLWW5scWJscHZNa3B5SzFVNE5IbDFjVTlyTjBZMFdubFRiMEoyTkdKWVNrZ3lXbEpTV2tab0wzVlRiSE5XT1hkU2JWbG9XWEoyT1RGRVdtbHhhemhJVWpaRVUyeHVabTVsZFRJNFJsUm9SVzF0YjNOVlRUTnJNbGxNYzBKak5FSnZkWEIwTTNsaFNEaFpia3BVTnpSMU16TjFlakU1TDAxNlZIVnFTMmMzVkdGcE1USXJXR0owYmxwRU9XcFVSMkY1U25Sc2FFWmxWeXRJUXpVM1FYUkJSbHBvY1ZsM2VVZHJXQ3M0TTBGaFVGaGFOR0V4VHpoMU1qTk9WVWQxTWtGd04yOU5NVTR3ZVVKS0swbHNUM29pTENKbElqb2lRVkZCUWlJc0ltRnNaeUk2SWxKVE1qVTJJaXdpYTJsa0lqb2lRVVJWTGpJeE1EWXdPUzVTTGxNaWZRLlJLS2VBZE02dGFjdWZpSVU3eTV2S3dsNFpQLURMNnEteHlrTndEdkljZFpIaTBIa2RIZ1V2WnoyZzZCTmpLS21WTU92dXp6TjhEczhybXo1dnMwT1RJN2tYUG1YeDZFLUYyUXVoUXNxT3J5LS1aN2J3TW5LYTNkZk1sbkthWU9PdURtV252RWMyR0hWdVVTSzREbmw0TE9vTTQxOVlMNThWTDAtSEthU18xYmNOUDhXYjVZR08xZXh1RmpiVGtIZkNIU0duVThJeUFjczlGTjhUT3JETHZpVEtwcWtvM3RiSUwxZE1TN3NhLWJkZExUVWp6TnVLTmFpNnpIWTdSanZGbjhjUDN6R2xjQnN1aVQ0XzVVaDZ0M05rZW1UdV9tZjdtZUFLLTBTMTAzMFpSNnNTR281azgtTE1sX0ZaUmh4djNFZFNtR2RBUTNlMDVMRzNnVVAyNzhTQWVzWHhNQUlHWmcxUFE3aEpoZGZHdmVGanJNdkdTSVFEM09wRnEtZHREcEFXbUo2Zm5sZFA1UWxYek5tQkJTMlZRQUtXZU9BYjh0Yjl5aVhsemhtT1dLRjF4SzlseHpYUG9GNmllOFRUWlJ4T0hxTjNiSkVISkVoQmVLclh6YkViV2tFNm4zTEoxbkd5M1htUlVFcER0Umdpa0tBUzZybFhFT0VneXNjIn0.eyJzaGEyNTYiOiJJVHB3ZWxTTjJRWHBBaDFDNVoxWjVDNmV3VzNHK2kvM1VKcm1kZ1lSaG1VPSJ9";
char ucBase64DecodedHeader[ 1400 ];
char ucBase64DecodedPayload[ 60 ];
char ucBase64DecodedSignature[ 400 ];

char ucBase64DecodedJWKHeader[ 48 ];
char ucBase64DecodedJWKPayload[ 700 ];
char ucBase64EncodedJWKPayloadCopyWithEquals[ 700 ];
char ucBase64DecodedJWKSignature[ 500 ];

char ucBase64DecodedSigningKeyN[ 512 ];
char ucBase64DecodedSigningKeyE[ 16 ];

char ucBase64EncodedCalculatedSignature[ 48 ];

char ucCalculatationBuffer[ 4096 ];

char * ucRandomSeed = "adu";

static uint32_t prvSplitJWS( char * pucJWS,
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

    return 0;
}

static void prvSwapToUrlEncodingChars( char * pucSignature,
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

uint32_t AzureIoT_SHA256Calculate( char * input,
                                   uint32_t inputLength,
                                   char * output )
{
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init( &ctx );
    mbedtls_md_setup( &ctx, mbedtls_md_info_from_type( md_type ), 0 );
    mbedtls_md_starts( &ctx );
    mbedtls_md_update( &ctx, ( const unsigned char * ) input, inputLength );
    mbedtls_md_finish( &ctx, output );
    mbedtls_md_free( &ctx );

    return 0;
}

uint32_t AzureIoT_RS256Verify( char * input,
                               uint32_t inputLength,
                               char * signature,
                               uint32_t signatureLength,
                               unsigned char * n,
                               uint32_t nLength,
                               unsigned char * e,
                               uint32_t eLength,
                               char * buffer,
                               uint32_t bufferLength )
{
    AzureIoTResult_t xResult;
    int mbedTLSResult;

    char * shaBuffer = buffer + azureiotMODULUS_SIZE;
    char * metadata;
    uint32_t metadataLength;

    char * decryptedPtr = buffer;
    size_t decryptedLength;

    printf("RS256 Verify Input:\n");
    printf("%.*s", inputLength, input);
    printf("\n");

    /* The signature is encrypted using the input key. We need to decrypt the */
    /* signature which gives us the SHA256. We then compare that to taking the SHA256 */
    /* of the input. */
    mbedtls_rsa_context ctx;

    mbedtls_rsa_init( &ctx, MBEDTLS_RSA_PKCS_V15, 0 );

    printf( "---- Initializing Decryption ----\n" );

    mbedTLSResult = mbedtls_rsa_import_raw( &ctx,
                                            n, nLength,
                                            NULL, 0,
                                            NULL, 0,
                                            NULL, 0,
                                            e, eLength );
    printf( "\tN Length: %i | E Length: %i\n", nLength, eLength );

    if( mbedTLSResult != 0 )
    {
        printf( "mbedtls res: %i\n", mbedTLSResult );
    }

    mbedTLSResult = mbedtls_rsa_complete( &ctx );

    if( mbedTLSResult != 0 )
    {
        printf( "mbedtls res: %i\n", mbedTLSResult );
    }

    mbedTLSResult = mbedtls_rsa_check_pubkey( &ctx );

    if( mbedTLSResult != 0 )
    {
        printf( "mbedtls res: %i\n", mbedTLSResult );
    }

    printf( "---- Decrypting ----\n" );

    /* RSA */
    mbedTLSResult = mbedtls_rsa_pkcs1_decrypt( &ctx, NULL, NULL, MBEDTLS_RSA_PUBLIC, &decryptedLength, signature, buffer, azureiotMODULUS_SIZE );

    if( mbedTLSResult != 0 )
    {
        printf( "mbedtls res: %i | %x\n", mbedTLSResult, -mbedTLSResult );
    }

    printf( "\tDecrypted text length: %li\n", decryptedLength );

    printf( "\tDecrypted text:\n" );
    int i = 0;

    while( i < decryptedLength )
    {
        printf( "0x%.2x ", ( unsigned char ) *( buffer + i ) );
        i++;
    }

    printf( "\n" );

    printf( "---- Calculating SHA256 over input ----\n" );
    xResult = AzureIoT_SHA256Calculate( input, inputLength,
                                        shaBuffer );

    printf( "\tCalculated: " );

    i = 0;

    while( i < azureiotSHA256_SIZE )
    {
        printf( "0x%.2x ", ( unsigned char ) *( shaBuffer + i ) );
        i++;
    }

    printf( "\n" );

    printf( "--- Checking for if SHA256 of header+payload matches decrypted SHA256 ---\n" );

    int doTheyMatch = memcmp( buffer + 19, shaBuffer, azureiotSHA256_SIZE );

    if( doTheyMatch == 0 )
    {
        printf( "\tSHA of JWK matches\n" );
        xResult = 0;
    }
    else
    {
        printf( "\tThey don't match\n" );
        xResult = 1;
    }

    return xResult;
}



uint32_t JWS_Verify( const char * pucEscapedManifest,
                     uint32_t ulEscapedManifestLength,
                     char * pucJWS,
                     uint32_t ulJWSLength )
{
    uint32_t ulVerificationResult;

    int mbedtResult;
    char * pucHeader;
    char * pucPayload;
    char * pucSignature;
    uint32_t ulHeaderLength;
    uint32_t ulPayloadLength;
    uint32_t ulSignatureLength;
    AzureIoTJSONReader_t xJSONReader;

    printf( "---------------------Begin Signature Validation --------------------\n\n" );

    memcpy( ucManifestBuffer, ucManifestSignature, strlen( ucManifestSignature ) );

    /*------------------- Parse and Decode the Manifest Sig ------------------------*/

    AzureIoTResult_t xResult = prvSplitJWS( pucJWS, ulJWSLength,
                                            &pucHeader, &ulHeaderLength,
                                            &pucPayload, &ulPayloadLength,
                                            &pucSignature, &ulSignatureLength );
    prvSwapToUrlEncodingChars( pucSignature, ulSignatureLength );

    printf( "---JWS Base64 Decode Header---\n" );
    int32_t outDecodedSizeOne;
    mbedtResult = mbedtls_base64_decode(ucBase64DecodedHeader, sizeof( ucBase64DecodedHeader ),(size_t*) &outDecodedSizeOne, pucHeader, ulHeaderLength);
    printf( "\tmbedTLS Return: 0x%x\n", mbedtResult );
    printf( "\tOut Decoded Size: %i\n", outDecodedSizeOne );
    printf( "\t%.*s\n\n", ( int ) outDecodedSizeOne, ucBase64DecodedHeader );

    printf( "---JWS Base64 Decode Payload---\n" );
    int32_t outDecodedSizeTwo;
    mbedtResult = mbedtls_base64_decode(ucBase64DecodedPayload, sizeof( ucBase64DecodedPayload ),(size_t*) &outDecodedSizeTwo, pucPayload, ulPayloadLength);
    printf( "\tmbedTLS Return: 0x%x\n", mbedtResult );
    printf( "\tOut Decoded Size: %i\n", outDecodedSizeTwo );
    printf( "\t%.*s\n\n", ( int ) outDecodedSizeTwo, ucBase64DecodedPayload );

    printf( "---JWS Base64 Decode Signature---\n" );
    int32_t outDecodedSizeThree;
    mbedtResult = mbedtls_base64_decode(ucBase64DecodedSignature, sizeof( ucBase64DecodedSignature ), (size_t*)&outDecodedSizeThree, pucSignature, ulSignatureLength);
    printf( "\tmbedTLS Return: 0x%x\n", mbedtResult );
    printf( "\tOut Decoded Size: %i\n", outDecodedSizeThree );
    printf( "\t%.*s\n\n", ( int ) outDecodedSizeThree, ucBase64DecodedSignature );


    /*------------------- Parse JSK JSON Payload ------------------------*/

    /* The "sjwk" is the signed signing public key */
    /* I believe as opposed to having a chain of trust for a public key, this is taking a known key */
    /* (baked into the device) and signing the key which was used to sign the manifest. */
    printf( "---Parsing JWS JSON Payload---\n" );

    //TODO: REMOVE THIS HACK
    ucBase64DecodedHeader[outDecodedSizeOne] = '"';
    ucBase64DecodedHeader[outDecodedSizeOne + 1] = '}';
    outDecodedSizeOne = outDecodedSizeOne + 2;
    AzureIoTJSONReader_Init( &xJSONReader, ucBase64DecodedHeader, outDecodedSizeOne );
    xResult = AzureIoTJSONReader_NextToken( &xJSONReader );

    while( xResult == eAzureIoTSuccess )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "sjwk", strlen( "sjwk" ) ) )
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            printf("Coreresult: %i\n", xResult);
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
    printf("JWKManifest Length: %i\n", ulJWKManifestLength);

    /*------------------- Base64 Decode the JWK Payload ------------------------*/

    char * pucJWKHeader;
    char * pucJWKPayload;
    char * pucJWKSignature;
    uint32_t ulJWKHeaderLength;
    uint32_t ulJWKPayloadLength;
    uint32_t ulJWKSignatureLength;

    printf( "--- Base64 Decoding JWS Payload ---\n" );

    xResult = prvSplitJWS( pucJWKManifest, ulJWKManifestLength,
                           &pucJWKHeader, &ulJWKHeaderLength,
                           &pucJWKPayload, &ulJWKPayloadLength,
                           &pucJWKSignature, &ulJWKSignatureLength );
    prvSwapToUrlEncodingChars( pucJWKSignature, ulJWKSignatureLength );

    printf( "--- JWK Base64 Decode Header ---\n" );
    int32_t outDecodedJWKSizeOne;
    mbedtls_base64_decode( ucBase64DecodedJWKHeader, sizeof( ucBase64DecodedJWKHeader ), (size_t*)&outDecodedJWKSizeOne, pucJWKHeader, ulJWKHeaderLength );
    printf( "\tCore Return: 0x%x\n", mbedtResult );
    printf( "\tOut Decoded Size: %i\n", outDecodedJWKSizeOne );
    printf( "\t%.*s\n\n", ( int ) outDecodedJWKSizeOne, ucBase64DecodedJWKHeader );

    printf( "--- JWK Base64 Decode Payload ---\n" );
    /* Have to hack in the padded characters */
    memcpy( ucBase64EncodedJWKPayloadCopyWithEquals, pucJWKPayload, ulJWKPayloadLength );
    ucBase64EncodedJWKPayloadCopyWithEquals[ ulJWKPayloadLength ] = '=';
    ucBase64EncodedJWKPayloadCopyWithEquals[ ulJWKPayloadLength + 1 ] = '=';
    int32_t newJWKPayloadLength = ulJWKPayloadLength + 2;

    int32_t outDecodedJWKSizeTwo;
    mbedtResult = mbedtls_base64_decode( ucBase64DecodedJWKPayload,
                                             sizeof( ucBase64DecodedJWKPayload ),
                                             (size_t*)&outDecodedJWKSizeTwo,
                                             ucBase64EncodedJWKPayloadCopyWithEquals,
                                             newJWKPayloadLength );
    printf( "\tCore Return: 0x%x\n", mbedtResult );
    printf( "\tOut Decoded Size: %i\n", outDecodedJWKSizeTwo );
    printf( "\t%.*s\n\n", ( int ) outDecodedJWKSizeTwo, ucBase64DecodedJWKPayload );

    printf( "--- JWK Base64 Decode Signature ---\n" );
    int32_t outDecodedJWKSizeThree;
    mbedtResult = mbedtls_base64_decode( ucBase64DecodedJWKSignature, sizeof( ucBase64DecodedJWKSignature ), (size_t*)&outDecodedJWKSizeThree, pucJWKSignature, ulJWKSignatureLength );
    printf( "\tCore Return: 0x%x\n", mbedtResult );
    printf( "\tOut Decoded Size: %i\n", outDecodedJWKSizeThree );
    printf( "\t%.*s\n\n", ( int ) outDecodedJWKSizeThree, ucBase64DecodedJWKSignature );

    /*------------------- Parse id for root key ------------------------*/

    printf( "--- Checking Root Key ---\n" );
    az_span kidSpan;
    AzureIoTJSONReader_Init( &xJSONReader, ucBase64DecodedJWKHeader, outDecodedJWKSizeOne );
    xResult = AzureIoTJSONReader_NextToken( &xJSONReader );

    while( xResult == eAzureIoTSuccess )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "kid", strlen( "kid" ) ) )
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            kidSpan = xJSONReader._internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
        else
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            xResult = AzureIoTJSONReader_SkipChildren( &xJSONReader );
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
    }

    az_span rootKeyIDSpan = az_span_create( ( uint8_t * ) AzureIoTADURootKeyId, sizeof( AzureIoTADURootKeyId ) - 1 );

    if( az_span_is_content_equal( rootKeyIDSpan, kidSpan ) )
    {
        printf( "\tUsing the correct root key\n" );
    }
    else
    {
        printf( "\tUsing the wrong root key\n" );

        while( 1 )
        {
        }
    }

    /*------------------- Parse necessary pieces for the verification ------------------------*/

    az_span nSpan;
    az_span eSpan;
    az_span algSpan;
    printf( "--- Parse Signing Key Payload ---\n" );

    AzureIoTJSONReader_Init( &xJSONReader, ucBase64DecodedJWKPayload, outDecodedJWKSizeTwo );
    xResult = AzureIoTJSONReader_NextToken( &xJSONReader );

    while( xResult == eAzureIoTSuccess )
    {
        if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "n", strlen( "n" ) ) )
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            nSpan = xJSONReader._internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
        else if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "e", strlen( "e" ) ) )
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            eSpan = xJSONReader._internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
        else if( AzureIoTJSONReader_TokenIsTextEqual( &xJSONReader, "alg", strlen( "alg" ) ) )
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            algSpan = xJSONReader._internal.xCoreReader.token.slice;

            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
        else
        {
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
            xResult = AzureIoTJSONReader_SkipChildren( &xJSONReader );
            xResult = AzureIoTJSONReader_NextToken( &xJSONReader );
        }
    }

    printf( "--- Print Signing Key Parts ---\n" );
    printf( "\tnSpan: %.*s\n", az_span_size( nSpan ), az_span_ptr( nSpan ) );
    printf( "\teSpan: %.*s\n", az_span_size( eSpan ), az_span_ptr( eSpan ) );
    printf( "\talgSpan: %.*s\n", az_span_size( algSpan ), az_span_ptr( algSpan ) );

    /*------------------- Base64 decode the key ------------------------*/
    printf( "--- Signing key base64 decoding N ---\n" );
    int32_t outDecodedSigningKeyN;
    mbedtResult = mbedtls_base64_decode( ucBase64DecodedSigningKeyN, sizeof( ucBase64DecodedSigningKeyN ) , (size_t*)&outDecodedSigningKeyN, az_span_ptr(nSpan), az_span_size(nSpan));
    printf( "\tmbedtResult Return: 0x%x\n", mbedtResult );
    printf( "\tOut Decoded Size: %i\n", outDecodedSigningKeyN );
    printf( "\t%.*s\n\n", ( int ) outDecodedSigningKeyN, ucBase64DecodedSigningKeyN );

    printf( "--- Signing key base64 decoding E ---\n" );
    int32_t outDecodedSigningKeyE;
    mbedtResult = mbedtls_base64_decode(ucBase64DecodedSigningKeyE, sizeof( ucBase64DecodedSigningKeyE ) , (size_t*)&outDecodedSigningKeyE, az_span_ptr(eSpan), az_span_size(eSpan));
    printf( "\tmbedtResult Return: 0x%x\n", mbedtResult );
    printf( "\tOut Decoded Size: %i\n", outDecodedSigningKeyE );
    printf( "\t%.*s\n\n", ( int ) outDecodedSigningKeyE, ucBase64DecodedSigningKeyE );


    /*------------------- Verify the signature ------------------------*/
    ulVerificationResult = AzureIoT_RS256Verify( pucJWKHeader, ulJWKHeaderLength + ulJWKPayloadLength + 1,
                                                 ucBase64DecodedJWKSignature, outDecodedJWKSizeThree,
                                                 ( unsigned char * ) AzureIoTADURootKeyN, sizeof( AzureIoTADURootKeyN ),
                                                 ( unsigned char * ) AzureIoTADURootKeyE, sizeof( AzureIoTADURootKeyE ),
                                                 ucCalculatationBuffer, sizeof( ucCalculatationBuffer ) );

    if( ulVerificationResult != 0 )
    {
        printf( "Verification of signing key failed\n" );
        return ulVerificationResult;
    }

    /*------------------- Verify that the signature was signed by signing key ------------------------*/
    ulVerificationResult = AzureIoT_RS256Verify( pucHeader, ulHeaderLength + ulPayloadLength + 1,
                                                 ucBase64DecodedSignature, outDecodedSizeThree,
                                                 ucBase64DecodedSigningKeyN, outDecodedSigningKeyN,
                                                 ucBase64DecodedSigningKeyE, outDecodedSigningKeyE,
                                                 ucCalculatationBuffer, sizeof( ucCalculatationBuffer ) );
    if( ulVerificationResult != 0 )
    {
        printf( "Verification of signed manifest SHA failed\n" );
        return ulVerificationResult;
    }

    /*------------------- Verify that the SHAs match ------------------------*/
    /* decodedSpanHeader */

    /*------------------- Done (Loop) ------------------------*/
    return ulVerificationResult;
}
