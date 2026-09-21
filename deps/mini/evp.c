/*
 * Minimal EVP digest shim: maps the EVP_MD API subset used by libfec
 * (fec_verity.cpp) onto the standalone SHA-1/SHA-256 implementations.
 */
#include <openssl/evp.h>
#include <openssl/obj_mac.h>
#include <openssl/sha.h>

#include <stdlib.h>
#include <string.h>

struct evp_md_st {
    int nid;
    size_t digest_size;
    void (*init)(void *ctx);
    void (*update)(void *ctx, const void *data, size_t len);
    void (*final)(void *ctx, unsigned char *md);
};

struct evp_md_ctx_st {
    const EVP_MD *md;
    void *state;
};

static void sha1_init_wrap(void *ctx)
{
    SHA1_Init((SHA_CTX *)ctx);
}

static void sha1_update_wrap(void *ctx, const void *data, size_t len)
{
    SHA1_Update((SHA_CTX *)ctx, data, len);
}

static void sha1_final_wrap(void *ctx, unsigned char *md)
{
    SHA1_Final(md, (SHA_CTX *)ctx);
}

static void sha256_init_wrap(void *ctx)
{
    SHA256_Init((SHA256_CTX *)ctx);
}

static void sha256_update_wrap(void *ctx, const void *data, size_t len)
{
    SHA256_Update((SHA256_CTX *)ctx, data, len);
}

static void sha256_final_wrap(void *ctx, unsigned char *md)
{
    SHA256_Final(md, (SHA256_CTX *)ctx);
}

static EVP_MD md_sha1 = {
    NID_sha1,
    SHA_DIGEST_LENGTH,
    sha1_init_wrap,
    sha1_update_wrap,
    sha1_final_wrap,
};

static EVP_MD md_sha256 = {
    NID_sha256,
    SHA256_DIGEST_LENGTH,
    sha256_init_wrap,
    sha256_update_wrap,
    sha256_final_wrap,
};

const EVP_MD *EVP_get_digestbynid(int nid)
{
    if (nid == NID_sha1) {
        return &md_sha1;
    }
    if (nid == NID_sha256) {
        return &md_sha256;
    }
    return NULL;
}

EVP_MD_CTX *EVP_MD_CTX_new(void)
{
    return (EVP_MD_CTX *)calloc(1, sizeof(EVP_MD_CTX));
}

void EVP_MD_CTX_free(EVP_MD_CTX *ctx)
{
    if (!ctx) {
        return;
    }
    free(ctx->state);
    free(ctx);
}

int EVP_DigestInit_ex(EVP_MD_CTX *ctx, const EVP_MD *type, void *impl)
{
    (void)impl;

    if (!ctx || !type) {
        return 0;
    }

    free(ctx->state);
    ctx->state = NULL;
    ctx->md = type;

    if (type->nid == NID_sha1) {
        ctx->state = malloc(sizeof(SHA_CTX));
    } else if (type->nid == NID_sha256) {
        ctx->state = malloc(sizeof(SHA256_CTX));
    } else {
        return 0;
    }
    if (!ctx->state) {
        return 0;
    }

    type->init(ctx->state);
    return 1;
}

int EVP_DigestUpdate(EVP_MD_CTX *ctx, const void *data, size_t count)
{
    if (!ctx || !ctx->md || !ctx->state) {
        return 0;
    }
    ctx->md->update(ctx->state, data, count);
    return 1;
}

int EVP_DigestFinal_ex(EVP_MD_CTX *ctx, unsigned char *md, unsigned int *s)
{
    if (!ctx || !ctx->md || !ctx->state) {
        return 0;
    }
    ctx->md->final(ctx->state, md);
    if (s) {
        *s = (unsigned int)ctx->md->digest_size;
    }
    return 1;
}
