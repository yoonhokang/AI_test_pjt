/**
 * @file mbedtls_config.h
 * @brief MbedTLS Configuration for STM32G474 (Embedded Optimized)
 */

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

// System Support
#define MBEDTLS_HAVE_ASM
#define MBEDTLS_NO_PLATFORM_ENTROPY

// Memory Configuration (Use FreeRTOS heap via mbedtls_port.c)
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_C
#include <stddef.h>
extern void* mbedtls_calloc(size_t n, size_t size);
extern void  mbedtls_free(void *ptr);
#define MBEDTLS_PLATFORM_CALLOC_MACRO mbedtls_calloc
#define MBEDTLS_PLATFORM_FREE_MACRO   mbedtls_free

// Protocol Support
#define MBEDTLS_SSL_PROTO_TLS1_2
#define MBEDTLS_SSL_PROTO_TLS1_3 
#define MBEDTLS_SSL_DTLS_ANTI_REPLAY

// Cipher Suites & Crypto
#define MBEDTLS_AES_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_MD_C
#define MBEDTLS_OID_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_RSA_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_HKDF_C  // Required for TLS 1.3 KDF
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_GCM_C   // Required for TLS 1.3 AEAD (AES-GCM)

// TLS 1.3 Requirements (ECDHE)
#define MBEDTLS_ECDH_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#define MBEDTLS_PKCS1_V15 // Required for TLS 1.2/1.3 backward compatibility (RSA)
#define MBEDTLS_PKCS1_V21 // Required for TLS 1.3 RSA signatures (PSS)

// TLS 1.3 Key Exchange Modes (Must be explicitly enabled if not auto-detected)
#define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED
#define MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_ENABLED

// Features
#define MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
#define MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_SRV_C
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_GENPRIME
#define MBEDTLS_ERROR_C
#define MBEDTLS_SSL_KEEP_PEER_CERTIFICATE
#define MBEDTLS_SSL_SESSION_TICKETS

// PSA Crypto (Required for MbedTLS 3.x)
#define MBEDTLS_PSA_CRYPTO_C
#define MBEDTLS_USE_PSA_CRYPTO
// #define MBEDTLS_PSA_CRYPTO_CONFIG // Disabled to allow legacy options to auto-configure PSA

// Buffer Sizes (Reduced for Embedded)
#define MBEDTLS_SSL_MAX_CONTENT_LEN     4096 
#define MBEDTLS_MPI_MAX_SIZE            512  // 4096-bit RSA

#include "main.h"

#endif /* MBEDTLS_CONFIG_H */
