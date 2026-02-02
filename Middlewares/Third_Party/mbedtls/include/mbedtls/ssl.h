/**
 * @file ssl.h
 * @brief MbedTLS SSL Header Stub
 */

#ifndef MBEDTLS_SSL_H
#define MBEDTLS_SSL_H

#include "config.h"
#include <stddef.h>
#include <stdint.h>

// Error Codes
#define MBEDTLS_ERR_SSL_WANT_READ   -0x004C
#define MBEDTLS_ERR_SSL_WANT_WRITE  -0x004E

// Constants
#define MBEDTLS_SSL_IS_CLIENT       0
#define MBEDTLS_SSL_TRANSPORT_STREAM 0
#define MBEDTLS_SSL_PRESET_DEFAULT  0

// Structures
typedef struct mbedtls_ssl_config { int dummy; } mbedtls_ssl_config;
typedef struct mbedtls_ssl_context { 
    void *p_bio; 
    int (*f_send)(void *, const unsigned char *, size_t);
    int (*f_recv)(void *, unsigned char *, size_t);
} mbedtls_ssl_context;
typedef struct mbedtls_entropy_context { int dummy; } mbedtls_entropy_context;
typedef struct mbedtls_ctr_drbg_context { int dummy; } mbedtls_ctr_drbg_context;

// API
void mbedtls_ssl_init(mbedtls_ssl_context *ssl);
void mbedtls_ssl_config_init(mbedtls_ssl_config *conf);
void mbedtls_ctr_drbg_init(mbedtls_ctr_drbg_context *ctx);
void mbedtls_entropy_init(mbedtls_entropy_context *ctx);

// Correct Signature for Seed
int mbedtls_ctr_drbg_seed(mbedtls_ctr_drbg_context *ctx,
                          int (*f_entropy)(void *, unsigned char *, size_t),
                          void *p_entropy,
                          const unsigned char *custom, size_t len);

// Random Gen Function
int mbedtls_ctr_drbg_random(void *p_rng, unsigned char *output, size_t output_len);

// Helper
int mbedtls_entropy_func(void *data, unsigned char *output, size_t len);

int mbedtls_ssl_config_defaults(mbedtls_ssl_config *conf,
                                int endpoint, int transport, int preset);

void mbedtls_ssl_conf_rng(mbedtls_ssl_config *conf,
                          int (*f_rng)(void *, unsigned char *, size_t),
                          void *p_rng);

int mbedtls_ssl_setup(mbedtls_ssl_context *ssl,
                      const mbedtls_ssl_config *conf);

void mbedtls_ssl_set_bio(mbedtls_ssl_context *ssl,
                         void *p_bio,
                         int (*f_send)(void *, const unsigned char *, size_t),
                         int (*f_recv)(void *, unsigned char *, size_t));

int mbedtls_ssl_handshake(mbedtls_ssl_context *ssl);

int mbedtls_ssl_write(mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len);
int mbedtls_ssl_read(mbedtls_ssl_context *ssl, unsigned char *buf, size_t len);

void mbedtls_ssl_free(mbedtls_ssl_context *ssl);
void mbedtls_ssl_config_free(mbedtls_ssl_config *conf);
void mbedtls_ctr_drbg_free(mbedtls_ctr_drbg_context *ctx);
void mbedtls_entropy_free(mbedtls_entropy_context *ctx);

#endif /* MBEDTLS_SSL_H */
