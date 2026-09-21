/*
 * SHA-256 implementation (FIPS 180-4), self-contained.
 * Written for the standalone fec build; algorithm is in the public domain.
 */

#include <openssl/sha.h>

#include <stdint.h>
#include <string.h>

typedef struct {
    uint32_t h[8];
    uint64_t Nl, Nh;
    uint8_t data[64];
    unsigned int num;
} sha256_state;

static const uint32_t K256[64] = {
    0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
    0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
    0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
    0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
    0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
    0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
    0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
    0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
    0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
    0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
    0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
    0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
    0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
    0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
    0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
    0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static void sha256_transform(sha256_state *st, const uint8_t block[64])
{
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h;
    int i;

    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i * 4] << 24) |
               ((uint32_t)block[i * 4 + 1] << 16) |
               ((uint32_t)block[i * 4 + 2] << 8) |
               ((uint32_t)block[i * 4 + 3]);
    }
    for (i = 16; i < 64; i++) {
        uint32_t s0 = ROTR(w[i - 15], 7) ^ ROTR(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = ROTR(w[i - 2], 17) ^ ROTR(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    a = st->h[0]; b = st->h[1]; c = st->h[2]; d = st->h[3];
    e = st->h[4]; f = st->h[5]; g = st->h[6]; h = st->h[7];

    for (i = 0; i < 64; i++) {
        uint32_t S1 = ROTR(e, 6) ^ ROTR(e, 11) ^ ROTR(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t t1 = h + S1 + ch + K256[i] + w[i];
        uint32_t S0 = ROTR(a, 2) ^ ROTR(a, 13) ^ ROTR(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = S0 + maj;
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    st->h[0] += a; st->h[1] += b; st->h[2] += c; st->h[3] += d;
    st->h[4] += e; st->h[5] += f; st->h[6] += g; st->h[7] += h;
}

static void sha256_init_state(sha256_state *st)
{
    st->h[0] = 0x6a09e667U;
    st->h[1] = 0xbb67ae85U;
    st->h[2] = 0x3c6ef372U;
    st->h[3] = 0xa54ff53aU;
    st->h[4] = 0x510e527fU;
    st->h[5] = 0x9b05688cU;
    st->h[6] = 0x1f83d9abU;
    st->h[7] = 0x5be0cd19U;
    st->Nl = 0;
    st->Nh = 0;
    st->num = 0;
}

static void sha256_block_data_order(sha256_state *st, const uint8_t *p, size_t num)
{
    while (num--) {
        sha256_transform(st, p);
        p += 64;
    }
}

static void sha256_update_impl(sha256_state *st, const void *data, size_t len)
{
    const uint8_t *p = data;
    size_t i;

    if (len == 0) {
        return;
    }

    i = st->Nl;
    st->Nl = st->Nl + (uint64_t)len * 8;
    if (st->Nl < i) {
        st->Nh++;
    }

    if (st->num) {
        size_t left = 64 - st->num;
        size_t chunk = len < left ? len : left;
        memcpy(st->data + st->num, p, chunk);
        st->num += chunk;
        p += chunk;
        len -= chunk;
        if (st->num == 64) {
            sha256_block_data_order(st, st->data, 1);
            st->num = 0;
        }
    }

    if (len >= 64) {
        size_t blocks = len / 64;
        sha256_block_data_order(st, p, blocks);
        p += blocks * 64;
        len -= blocks * 64;
    }

    if (len) {
        memcpy(st->data, p, len);
        st->num = (unsigned int)len;
    }
}

static void sha256_final_impl(sha256_state *st, uint8_t md[32])
{
    uint64_t bitlen = st->Nl | ((uint64_t)st->Nh << 32);
    uint8_t pad[128];
    size_t i, padlen;

    memset(pad, 0, sizeof(pad));
    pad[0] = 0x80;
    padlen = (st->num < 56) ? (56 - st->num) : (120 - st->num);
    padlen += 8;

    for (i = 0; i < 8; i++) {
        pad[padlen - 1 - i] = (uint8_t)(bitlen >> (i * 8));
    }

    sha256_update_impl(st, pad, padlen);

    for (i = 0; i < 8; i++) {
        md[i * 4] = (uint8_t)(st->h[i] >> 24);
        md[i * 4 + 1] = (uint8_t)(st->h[i] >> 16);
        md[i * 4 + 2] = (uint8_t)(st->h[i] >> 8);
        md[i * 4 + 3] = (uint8_t)st->h[i];
    }
}

int SHA256_Init(SHA256_CTX *c)
{
    sha256_init_state((sha256_state *)c);
    return 1;
}

int SHA256_Update(SHA256_CTX *c, const void *data, size_t len)
{
    sha256_update_impl((sha256_state *)c, data, len);
    return 1;
}

int SHA256_Final(uint8_t *md, SHA256_CTX *c)
{
    sha256_final_impl((sha256_state *)c, md);
    return 1;
}

uint8_t *SHA256(const uint8_t *d, size_t n, uint8_t *md)
{
    SHA256_CTX c;
    SHA256_Init(&c);
    SHA256_Update(&c, d, n);
    SHA256_Final(md, &c);
    return md;
}
