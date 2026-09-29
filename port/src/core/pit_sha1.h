#ifndef PIT_CORE_PIT_SHA1_H
#define PIT_CORE_PIT_SHA1_H

#include <stddef.h>
#include <stdint.h>

#define PIT_SHA1_DIGEST_LEN 20
#define PIT_SHA1_HEX_LEN    41

typedef struct {
    uint32_t      state[5];
    uint64_t      count;
    unsigned char buffer[64];
} pit_sha1_ctx;

void pit_sha1_init(pit_sha1_ctx *ctx);
void pit_sha1_update(pit_sha1_ctx *ctx, const void *data, size_t len);
void pit_sha1_final(pit_sha1_ctx *ctx, unsigned char digest[PIT_SHA1_DIGEST_LEN]);

int pit_sha1_file_hex(const char *path, char out[PIT_SHA1_HEX_LEN]);

#endif
