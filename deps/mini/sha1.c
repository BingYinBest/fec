/*
 * SHA-1 implementation (FIPS 180-1), self-contained.
 * Written for the standalone fec build; algorithm is in the public domain.
 */

#include <openssl/sha.h>

#include <stdint.h>
#include <string.h>

typedef struct {
    uint32_t h[5];
    uint64_t Nl, Nh;
    uint8_t data[64];
    unsigned int num;
} sha1_state;

#define ROL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

static void sha1_transform(sha1_state *st, const uint8_t block[64])
{
    uint32_t w[80];
    uint32_t a, b, c, d, e;
    int i;

    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i * 4] << 24) |
               ((uint32_t)block[i * 4 + 1] << 16) |
               ((uint32_t)block[i * 4 + 2] << 8) |
               ((uint32_t)block[i * 4 + 3]);
    }
    for (i = 16; i < 80; i++) {
        w[i] = ROL(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    a = st->h[0]; b = st->h[1]; c = st->h[2]; d = st->h[3]; e = st->h[4];

    for (i = 0; i < 80; i++) {
        uint32_t f, k, tmp;

        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5a827999U;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ed9eba1U;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8f1bbcdcU;
        } else {
            f = b ^ c ^ d;
            k = 0xca62c1d6U;
        }

        tmp = ROL(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = ROL(b, 30);
        b = a;
        a = tmp;
    }

    st->h[0] += a; st->h[1] += b; st->h[2] += c; st->h[3] += d; st->h[4] += e;
}

static void sha1_init_state(sha1_state *st)
{
    st->h[0] = 0x67452301U;
    st->h[1] = 0xefcdab89U;
    st->h[2] = 0x98badcfeU;
    st->h[3] = 0x10325476U;
    st->h[4] = 0xc3d2e1f0U;
    st->Nl = 0;
    st->Nh = 0;
    st->num = 0;
}

static void sha1_block_data_order(sha1_state *st, const uint8_t *p, size_t num)
{
    while (num--) {
        sha1_transform(st, p);
        p += 64;
    }
}

static void sha1_update_impl(sha1_state *st, const void *data, size_t len)
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
            sha1_block_data_order(st, st->data, 1);
            st->num = 0;
        }
    }

    if (len >= 64) {
        size_t blocks = len / 64;
        sha1_block_data_order(st, p, blocks);
        p += blocks * 64;
        len -= blocks * 64;
    }

    if (len) {
        memcpy(st->data, p, len);
        st->num = (unsigned int)len;
    }
}

static void sha1_final_impl(sha1_state *st, uint8_t md[20])
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

    sha1_update_impl(st, pad, padlen);

    for (i = 0; i < 5; i++) {
        md[i * 4] = (uint8_t)(st->h[i] >> 24);
        md[i * 4 + 1] = (uint8_t)(st->h[i] >> 16);
        md[i * 4 + 2] = (uint8_t)(st->h[i] >> 8);
        md[i * 4 + 3] = (uint8_t)st->h[i];
    }
}

int SHA1_Init(SHA_CTX *c)
{
    sha1_init_state((sha1_state *)c);
    return 1;
}

int SHA1_Update(SHA_CTX *c, const void *data, size_t len)
{
    sha1_update_impl((sha1_state *)c, data, len);
    return 1;
}

int SHA1_Final(uint8_t *md, SHA_CTX *c)
{
    sha1_final_impl((sha1_state *)c, md);
    return 1;
}

uint8_t *SHA1(const uint8_t *d, size_t n, uint8_t *md)
{
    SHA_CTX c;
    SHA1_Init(&c);
    SHA1_Update(&c, d, n);
    SHA1_Final(md, &c);
    return md;
}
