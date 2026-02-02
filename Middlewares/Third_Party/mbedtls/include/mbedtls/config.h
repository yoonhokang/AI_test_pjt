/**
 * @file config.h
 * @brief MbedTLS Configuration for STM32G474
 */

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

// System Support
#define MBEDTLS_HAVE_ASM
#define MBEDTLS_NO_PLATFORM_ENTROPY

// Memory Configuration
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_C
#include <stddef.h>
extern void* mbedtls_calloc(size_t n, size_t size);
extern void  mbedtls_free(void *ptr);
#define MBEDTLS_PLATFORM_CALLOC_MACRO mbedtls_calloc
#define MBEDTLS_PLATFORM_FREE_MACRO   mbedtls_free

// Protocol Support
#define MBEDTLS_SSL_PROTO_TLS1_2
// Disable older/newer versions to save space
// #define MBEDTLS_SSL_PROTO_TLS1_0 (unsafe)
// #define MBEDTLS_SSL_PROTO_TLS1_3 (too heavy)

// Cipher Suites (Restrict to standard strong ones)
#define MBEDTLS_SSL_CIPHERSUITES \
    MBEDTLS_TLS_RSA_WITH_AES_128_CBC_SHA256, \
    MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA256

// Features
#define MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
#define MBEDTLS_AES_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_MD_C
#define MBEDTLS_OID_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_RSA_C
#define MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_SRV_C
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C

// Buffer Sizes (Reduced for Embedded)
#define MBEDTLS_SSL_MAX_CONTENT_LEN     4096 // Incoming fragments
#define MBEDTLS_MPI_MAX_SIZE            512  // 4096-bit RSA support

#include "main.h" // For FreeRTOS headers if needed for macros

#endif /* MBEDTLS_CONFIG_H */
