#include "core/pit_png.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PNG_SIGNATURE_SIZE 8

static const unsigned char k_signature[PNG_SIGNATURE_SIZE] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
};

static unsigned int crc_table[256];
static int crc_table_ready;

static void crc_table_build(void)
{
    unsigned int n, c, k;

    for (n = 0; n < 256; n++) {
        c = n;
        for (k = 0; k < 8; k++) {
            c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        }
        crc_table[n] = c;
    }
    crc_table_ready = 1;
}

static unsigned int crc32_update(unsigned int crc, const unsigned char *buf, size_t len)
{
    size_t i;

    if (!crc_table_ready) {
        crc_table_build();
    }
    for (i = 0; i < len; i++) {
        crc = crc_table[(crc ^ buf[i]) & 0xFFu] ^ (crc >> 8);
    }
    return crc;
}

static void put_be32(unsigned char *p, unsigned int v)
{
    p[0] = (unsigned char)(v >> 24);
    p[1] = (unsigned char)(v >> 16);
    p[2] = (unsigned char)(v >> 8);
    p[3] = (unsigned char)(v);
}

static int write_chunk(FILE *f, const char *type, const unsigned char *data,
                       size_t len)
{
    unsigned char header[8];
    unsigned char trailer[4];
    unsigned int crc;

    put_be32(header, (unsigned int)len);
    memcpy(header + 4, type, 4);

    if (fwrite(header, 1, 8, f) != 8) {
        return -1;
    }
    if (len > 0 && fwrite(data, 1, len, f) != len) {
        return -1;
    }

    crc = crc32_update(0xFFFFFFFFu, (const unsigned char *)type, 4);
    crc = crc32_update(crc, data, len) ^ 0xFFFFFFFFu;
    put_be32(trailer, crc);

    return fwrite(trailer, 1, 4, f) == 4 ? 0 : -1;
}

static unsigned int adler32(const unsigned char *data, size_t len)
{
    unsigned int a = 1, b = 0;
    size_t i;

    for (i = 0; i < len; i++) {
        a = (a + data[i]) % 65521u;
        b = (b + a) % 65521u;
    }
    return (b << 16) | a;
}

int pit_png_write(const char *path, const pit_image *image)
{
    FILE *f;
    unsigned char ihdr[13];
    unsigned char *raw = NULL;
    unsigned char *zdata = NULL;
    size_t raw_len, zlen, zcap, pos;
    size_t stride;
    int y, result = -1;

    if (path == NULL || image == NULL || image->pixels == NULL ||
        image->width <= 0 || image->height <= 0) {
        return -1;
    }

    stride = (size_t)image->width * 4u;
    raw_len = ((size_t)image->height * (stride + 1u));

    raw = (unsigned char *)malloc(raw_len);
    if (raw == NULL) {
        return -1;
    }
    for (y = 0; y < image->height; y++) {
        unsigned char *row = raw + (size_t)y * (stride + 1u);
        row[0] = 0; /* filter type: none */
        if (pit_image_row_to_rgba8(image, y, row + 1) != 0) {
            goto done;
        }
    }

    /* zlib header, then stored deflate blocks, then the adler32 checksum. */
    zcap = 2u + raw_len + ((raw_len / 65535u) + 1u) * 5u + 4u;
    zdata = (unsigned char *)malloc(zcap);
    if (zdata == NULL) {
        goto done;
    }

    zlen = 0;
    zdata[zlen++] = 0x78;
    zdata[zlen++] = 0x01;

    pos = 0;
    do {
        size_t block = raw_len - pos;
        int final;

        if (block > 65535u) {
            block = 65535u;
        }
        final = (pos + block >= raw_len) ? 1 : 0;

        zdata[zlen++] = (unsigned char)final;
        zdata[zlen++] = (unsigned char)(block & 0xFFu);
        zdata[zlen++] = (unsigned char)(block >> 8);
        zdata[zlen++] = (unsigned char)(~block & 0xFFu);
        zdata[zlen++] = (unsigned char)((~block >> 8) & 0xFFu);

        memcpy(zdata + zlen, raw + pos, block);
        zlen += block;
        pos += block;
    } while (pos < raw_len);

    put_be32(zdata + zlen, adler32(raw, raw_len));
    zlen += 4u;

    f = fopen(path, "wb");
    if (f == NULL) {
        goto done;
    }

    put_be32(ihdr, (unsigned int)image->width);
    put_be32(ihdr + 4, (unsigned int)image->height);
    ihdr[8] = 8;   /* bit depth */
    ihdr[9] = 6;   /* colour type: truecolour with alpha */
    ihdr[10] = 0;  /* compression: deflate */
    ihdr[11] = 0;  /* filter method: adaptive */
    ihdr[12] = 0;  /* interlace: none */

    if (fwrite(k_signature, 1, PNG_SIGNATURE_SIZE, f) != PNG_SIGNATURE_SIZE ||
        write_chunk(f, "IHDR", ihdr, sizeof(ihdr)) != 0 ||
        write_chunk(f, "IDAT", zdata, zlen) != 0 ||
        write_chunk(f, "IEND", NULL, 0) != 0) {
        fclose(f);
        goto done;
    }

    result = (fclose(f) == 0) ? 0 : -1;

done:
    free(raw);
    free(zdata);
    return result;
}
