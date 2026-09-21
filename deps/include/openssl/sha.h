/*
 * Minimal SHA-1/SHA-256 API compatible with the subset used by libfec.
 * Standalone implementation; see deps/mini/sha1.c and deps/mini/sha256.c.
 */
#ifndef OPENSSL_HEADER_SHA_H
#define OPENSSL_HEADER_SHA_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SHA_DIGEST_LENGTH 20
#define SHA256_DIGEST_LENGTH 32

typedef struct SHA_CTX {
    uint32_t h[5];
    uint64_t Nl, Nh;
    uint8_t data[64];
    unsigned int num;
} SHA_CTX;

int SHA1_Init(SHA_CTX *c);
int SHA1_Update(SHA_CTX *c, const void *data, size_t len);
int SHA1_Final(uint8_t *md, SHA_CTX *c);
uint8_t *SHA1(const uint8_t *data, size_t len, uint8_t *out);

typedef struct SHA256_CTX {
    uint32_t h[8];
    uint64_t Nl, Nh;
    uint8_t data[64];
    unsigned int num;
} SHA256_CTX;

int SHA256_Init(SHA256_CTX *c);
int SHA256_Update(SHA256_CTX *c, const void *data, size_t len);
int SHA256_Final(uint8_t *md, SHA256_CTX *c);
uint8_t *SHA256(const uint8_t *data, size_t len, uint8_t *out);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_HEADER_SHA_H */
