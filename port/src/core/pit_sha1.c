#include "core/pit_sha1.h"

#include <stdio.h>
#include <string.h>

#define ROL(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

static void pit_sha1_transform(pit_sha1_ctx *ctx, const unsigned char block[64])
{
    uint32_t w[80];
    uint32_t a, b, c, d, e;
    int i;

    for (i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i * 4 + 0] << 24)
             | ((uint32_t)block[i * 4 + 1] << 16)
             | ((uint32_t)block[i * 4 + 2] << 8)
             | ((uint32_t)block[i * 4 + 3]);
    }
    for (i = 16; i < 80; i++) {
        w[i] = ROL(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];

    for (i = 0; i < 80; i++) {
        uint32_t f;
        uint32_t k;
        uint32_t temp;

        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999u;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1u;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDCu;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6u;
        }

        temp = ROL(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = ROL(b, 30);
        b = a;
        a = temp;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
}

void pit_sha1_init(pit_sha1_ctx *ctx)
{
    ctx->state[0] = 0x67452301u;
    ctx->state[1] = 0xEFCDAB89u;
    ctx->state[2] = 0x98BADCFEu;
    ctx->state[3] = 0x10325476u;
    ctx->state[4] = 0xC3D2E1F0u;
    ctx->count = 0;
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
}

void pit_sha1_update(pit_sha1_ctx *ctx, const void *data, size_t len)
{
    const unsigned char *bytes = (const unsigned char *)data;
    size_t have = (size_t)((ctx->count >> 3) & 63);
    size_t need;

    ctx->count += (uint64_t)len * 8u;

    if (have != 0) {
        need = 64 - have;
        if (len < need) {
            memcpy(ctx->buffer + have, bytes, len);
            return;
        }
        memcpy(ctx->buffer + have, bytes, need);
        pit_sha1_transform(ctx, ctx->buffer);
        bytes += need;
        len -= need;
    }

    while (len >= 64) {
        pit_sha1_transform(ctx, bytes);
        bytes += 64;
        len -= 64;
    }

    if (len > 0) {
        memcpy(ctx->buffer, bytes, len);
    }
}

void pit_sha1_final(pit_sha1_ctx *ctx, unsigned char digest[PIT_SHA1_DIGEST_LEN])
{
    unsigned char pad[72];
    size_t have = (size_t)((ctx->count >> 3) & 63);
    size_t padlen = (have < 56) ? (56 - have) : (120 - have);
    uint64_t bits = ctx->count;
    int i;

    memset(pad, 0, sizeof(pad));
    pad[0] = 0x80;
    for (i = 0; i < 8; i++) {
        pad[padlen + i] = (unsigned char)(bits >> (56 - 8 * i));
    }

    pit_sha1_update(ctx, pad, padlen + 8);

    for (i = 0; i < PIT_SHA1_DIGEST_LEN; i++) {
        digest[i] = (unsigned char)(ctx->state[i >> 2] >> (24 - 8 * (i & 3)));
    }
}

int pit_sha1_file_hex(const char *path, char out[PIT_SHA1_HEX_LEN])
{
    unsigned char chunk[65536];
    unsigned char digest[PIT_SHA1_DIGEST_LEN];
    pit_sha1_ctx ctx;
    size_t n;
    int i;
    FILE *f = fopen(path, "rb");

    if (f == NULL) {
        return -1;
    }

    pit_sha1_init(&ctx);
    while ((n = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        pit_sha1_update(&ctx, chunk, n);
    }

    if (ferror(f)) {
        fclose(f);
        return -1;
    }
    fclose(f);

    pit_sha1_final(&ctx, digest);

    for (i = 0; i < PIT_SHA1_DIGEST_LEN; i++) {
        snprintf(out + i * 2, 3, "%02x", digest[i]);
    }
    out[PIT_SHA1_DIGEST_LEN * 2] = '\0';
    return 0;
}
