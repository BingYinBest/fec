/*
 * Minimal EVP digest API compatible with the subset used by libfec.
 * Implementation in deps/mini/evp.c.
 */
#ifndef OPENSSL_HEADER_EVP_H
#define OPENSSL_HEADER_EVP_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct evp_md_st EVP_MD;
typedef struct evp_md_ctx_st EVP_MD_CTX;

const EVP_MD *EVP_get_digestbynid(int nid);
EVP_MD_CTX *EVP_MD_CTX_new(void);
void EVP_MD_CTX_free(EVP_MD_CTX *ctx);
int EVP_DigestInit_ex(EVP_MD_CTX *ctx, const EVP_MD *type, void *impl);
int EVP_DigestUpdate(EVP_MD_CTX *ctx, const void *data, size_t count);
int EVP_DigestFinal_ex(EVP_MD_CTX *ctx, unsigned char *md, unsigned int *s);

#ifdef __cplusplus
}
#endif

#endif /* OPENSSL_HEADER_EVP_H */
