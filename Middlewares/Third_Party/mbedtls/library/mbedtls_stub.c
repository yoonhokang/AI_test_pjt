/**
 * @file mbedtls_stub.c
 * @brief MbedTLS Mock/Stub Implementation
 */

#include "mbedtls/ssl.h"
#include <stdio.h>

// --- Init/Free ---
void mbedtls_ssl_init(mbedtls_ssl_context *ssl) { (void)ssl; }
void mbedtls_ssl_config_init(mbedtls_ssl_config *conf) { (void)conf; }
void mbedtls_ctr_drbg_init(mbedtls_ctr_drbg_context *ctx) { (void)ctx; }
void mbedtls_entropy_init(mbedtls_entropy_context *ctx) { (void)ctx; }

void mbedtls_ssl_free(mbedtls_ssl_context *ssl) { (void)ssl; }
void mbedtls_ssl_config_free(mbedtls_ssl_config *conf) { (void)conf; }
void mbedtls_ctr_drbg_free(mbedtls_ctr_drbg_context *ctx) { (void)ctx; }
void mbedtls_entropy_free(mbedtls_entropy_context *ctx) { (void)ctx; }

// --- Configuration ---

// Standard Entropy Function (Stub)
// Signature: int f(void *data, unsigned char *output, size_t len)
int mbedtls_entropy_func(void *data, unsigned char *output, size_t len)
{
    // Stub: fill with varying data or just 0
    (void)data;
    for(size_t i=0; i<len; i++) output[i] = (unsigned char)i;
    return 0;
}

int mbedtls_ctr_drbg_seed(mbedtls_ctr_drbg_context *ctx,
                          int (*f_entropy)(void *, unsigned char *, size_t),
                          void *p_entropy,
                          const unsigned char *custom, size_t len)
{
    // In real code, we would call f_entropy to seed.
    return 0; // Success
}

// Random Generator Stub
int mbedtls_ctr_drbg_random(void *p_rng, unsigned char *output, size_t output_len)
{
    (void)p_rng;
    // Just return success (0). 
    // In stub, maybe fill with dummy data
    for(size_t i=0; i<output_len; i++) output[i] = (unsigned char)i;
    return 0;
}

int mbedtls_ssl_config_defaults(mbedtls_ssl_config *conf,
                                int endpoint, int transport, int preset)
{
    return 0; // Success
}

void mbedtls_ssl_conf_rng(mbedtls_ssl_config *conf,
                          int (*f_rng)(void *, unsigned char *, size_t),
                          void *p_rng)
{
}

int mbedtls_ssl_setup(mbedtls_ssl_context *ssl, const mbedtls_ssl_config *conf)
{
    return 0; // Success
}

void mbedtls_ssl_set_bio(mbedtls_ssl_context *ssl,
                         void *p_bio,
                         int (*f_send)(void *, const unsigned char *, size_t),
                         int (*f_recv)(void *, unsigned char *, size_t))
{
    ssl->p_bio = p_bio;
    ssl->f_send = f_send;
    ssl->f_recv = f_recv;
}

// --- Operations ---

int mbedtls_ssl_handshake(mbedtls_ssl_context *ssl)
{
    printf("[MbedTLS] Mock Handshake Success.\r\n");
    return 0; 
}

int mbedtls_ssl_write(mbedtls_ssl_context *ssl, const unsigned char *buf, size_t len)
{
    // Pass-through to BIO (W5500_Send)
    if (ssl->f_send)
    {
        return ssl->f_send(ssl->p_bio, buf, len);
    }
    return -1;
}

int mbedtls_ssl_read(mbedtls_ssl_context *ssl, unsigned char *buf, size_t len)
{
    // Pass-through to BIO (W5500_Recv)
    if (ssl->f_recv)
    {
        return ssl->f_recv(ssl->p_bio, buf, len);
    }
    return -1;
}
