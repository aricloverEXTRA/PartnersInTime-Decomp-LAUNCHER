#include "core/pit_nds_banner.h"

#include <stdlib.h>
#include <string.h>

/*
 * Cartridge banner layout.
 *
 * Offsets 0x004..0x01F are reserved padding, which is why the icon bitmap
 * begins at 0x020 and not at 0x010: reading from 0x010 silently picks up 16
 * zero bytes in front of the real data and renders a blank strip down the side
 * of the image. The 4bpp icon and its palette sit at the same offsets in every
 * banner version; only the titles move.
 */

#define BANNER_RESERVED_SIZE   0x020
#define BANNER_ICON_OFFSET     0x020
#define BANNER_ICON_BYTES      0x200
#define BANNER_PALETTE_OFFSET  0x220

#define BANNER_TITLES_V1       0x240
#define BANNER_TITLES_V2       0x300
#define BANNER_TITLE_STRIDE    0x100
#define BANNER_TITLE_SIZE      0x100

#define BANNER_MIN_SIZE        0x240

#define BANNER_ICON_BYTES_V1    0x200
#define BANNER_ICON_BYTES_V2    0x220
unsigned int pit_banner_crc16(const unsigned char *data, size_t len)
{
    unsigned int crc = 0xFFFFu;
    size_t i;
    int b;

    for (i = 0; i < len; i++) {
        crc ^= (unsigned int)data[i] << 8;
        for (b = 0; b < 8; b++) {
            crc = (crc & 0x8000u) ? ((crc << 1) ^ 0x1021u) : (crc << 1);
        }
    }
    return crc & 0xFFFFu;
}

static unsigned int read_u16(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

/*
 * Copies a fixed-width, NUL-padded field into a C string.
 *
 * The bytes are passed through unchanged. Nothing here interprets an encoding,
 * it only finds the terminator and copies; text conversion is a display concern.
 */
static void copy_field(char *dst, size_t dst_size, const unsigned char *src,
                       size_t src_size)
{
    size_t n = 0;

    while (n < src_size && src[n] != 0x00u) {
        n++;
    }
    if (n >= dst_size) {
        n = dst_size - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
}

/*
 * Distinguishes UTF-16LE text from a single-byte encoding.
 *
 * The banner format documents the first title field as Shift-JIS and the rest
 * as UTF-16LE, but that is not reliable: this game's Japanese field holds the
 * same UTF-16LE English string as the English field, because the release has no
 * Japanese text. Assuming Shift-JIS there yields a single stray character.
 *
 * In UTF-16LE ASCII the high byte of each code unit is zero, so several zero
 * bytes appear at odd offsets within the first few units. A real Shift-JIS
 * title has both bytes of every character set, so this does not misfire on it.
 */
static int looks_utf16le(const unsigned char *src, size_t size)
{
    size_t i, units = 0, limit = size < 16 ? size : 16;

    for (i = 0; i + 1 < limit; i += 2) {
        if (src[i] == 0x00u && src[i + 1] != 0x00u) {
            return 0;
        }
        if (src[i + 1] == 0x00u) {
            units++;
        }
    }
    return units >= 2;
}

/*
 * Copies a UTF-16LE field and converts it to UTF-8.
 *
 * A plain NUL scan cannot be used here: every ASCII code unit in UTF-16LE has
 * a zero high byte, so a byte-wise scan stops at the first character. This
 * scans for a zero *code unit* instead, skips a byte order mark, and folds
 * newlines to spaces so a title stays on one line in a log.
 */
static void copy_utf16_field(char *dst, size_t dst_size, const unsigned char *src,
                             size_t src_size)
{
    size_t units, i, out = 0;

    if (src_size < 2) {
        dst[0] = '\0';
        return;
    }
    if (src[0] == 0xFFu && src[1] == 0xFEu) {
        src += 2;
        src_size -= 2;
    }

    units = src_size / 2;
    for (i = 0; i < units; i++) {
        unsigned int c = (unsigned int)src[i * 2] | ((unsigned int)src[i * 2 + 1] << 8);

        if (c == 0u) {
            break;
        }
        if (c == '\r' || c == '\n') {
            c = ' ';
        }

        if (c < 0x80u) {
            if (out + 1u >= dst_size) {
                break;
            }
            dst[out++] = (char)c;
        } else if (c < 0x800u) {
            if (out + 2u >= dst_size) {
                break;
            }
            dst[out++] = (char)(0xC0u | (c >> 6));
            dst[out++] = (char)(0x80u | (c & 0x3Fu));
        } else {
            if (out + 3u >= dst_size) {
                break;
            }
            dst[out++] = (char)(0xE0u | (c >> 12));
            dst[out++] = (char)(0x80u | ((c >> 6) & 0x3Fu));
            dst[out++] = (char)(0x80u | (c & 0x3Fu));
        }
    }

    dst[out] = '\0';
}

/*
 * Copies one title field, detecting UTF-16LE and falling back to a raw byte copy
 * for single-byte encodings such as Shift-JIS. Raw bytes are not interpreted, so
 * a Shift-JIS title is reported verbatim rather than mis-decoded.
 */
static void copy_title(char *dst, size_t dst_size, const unsigned char *src,
                       size_t src_size)
{
    if (looks_utf16le(src, src_size)) {
        copy_utf16_field(dst, dst_size, src, src_size);
    } else {
        copy_field(dst, dst_size, src, src_size);
    }
}

int pit_banner_decode(pit_banner *banner, const pit_rom *rom)
{
    const unsigned char *base;
    unsigned int size, version;
    unsigned int icon_offset, palette_offset, title_base;
    int i;

    if (banner == NULL || rom == NULL) {
        return -1;
    }
    memset(banner, 0, sizeof(*banner));

    if (rom->banner_offset == 0 || rom->banner_size < BANNER_MIN_SIZE) {
        return -1;
    }

    base = rom->data + rom->banner_offset;
    size = rom->banner_size;

    version = read_u16(base);
    if (version == 0u) {
        return -1;
    }

    memset(banner, 0, sizeof(*banner));
    banner->version = version;
    banner->offset = rom->banner_offset;
    banner->size = size;
    banner->crc_reproducible = 1;
    banner->crc16_stored = read_u16(base + 2);
    banner->crc16_computed = pit_banner_crc16(base + 2, size - 2u);
    banner->crc_ok = (banner->crc16_stored == banner->crc16_computed);
    if (!banner->crc_ok) {
        banner->crc_reproducible = 0;
    }

    icon_offset = BANNER_ICON_OFFSET;
    palette_offset = BANNER_PALETTE_OFFSET;
    title_base = (version == 0x0001u) ? BANNER_TITLES_V1 : BANNER_TITLES_V2;

    if ((size_t)title_base + (size_t)BANNER_TITLE_STRIDE * 5u + BANNER_TITLE_SIZE > size) {
        return -1;
    }
    if ((size_t)palette_offset + PIT_BANNER_COLORS * 2u > size) {
        return -1;
    }
    if ((size_t)icon_offset + BANNER_ICON_BYTES > size) {
        return -1;
    }

    for (i = 0; i < PIT_BANNER_COLORS; i++) {
        banner->palette[i] = pit_bgr555(255, read_u16(base + palette_offset + i * 2));
    }

    if (pit_tiles_decode(&banner->icon, base + icon_offset, 4,
                         PIT_BANNER_ICON_TILES_X, PIT_BANNER_ICON_TILES_Y,
                         banner->palette, PIT_BANNER_COLORS, 0) != 0) {
        return -1;
    }

    banner->icon_scale = 4;
    if (pit_image_scale(&banner->icon_scaled, &banner->icon, banner->icon_scale) != 0) {
        pit_image_free(&banner->icon);
        return -1;
    }

    copy_title(banner->japanese_title, sizeof(banner->japanese_title),
               base + title_base, BANNER_TITLE_SIZE);
    copy_title(banner->english_title, sizeof(banner->english_title),
               base + title_base + BANNER_TITLE_STRIDE, BANNER_TITLE_SIZE);
    copy_title(banner->french_title, sizeof(banner->french_title),
               base + title_base + BANNER_TITLE_STRIDE * 2u, BANNER_TITLE_SIZE);
    copy_title(banner->german_title, sizeof(banner->german_title),
               base + title_base + BANNER_TITLE_STRIDE * 3u, BANNER_TITLE_SIZE);
    copy_title(banner->italian_title, sizeof(banner->italian_title),
               base + title_base + BANNER_TITLE_STRIDE * 4u, BANNER_TITLE_SIZE);
    copy_title(banner->spanish_title, sizeof(banner->spanish_title),
               base + title_base + BANNER_TITLE_STRIDE * 5u, BANNER_TITLE_SIZE);

    return 0;
}

void pit_banner_free(pit_banner *banner)
{
    if (banner == NULL) {
        return;
    }
    pit_image_free(&banner->icon);
    pit_image_free(&banner->icon_scaled);
}

int pit_banner_draw(const pit_banner *banner, pit_screen *screen)
{
    int x, y;

    if (banner == NULL || screen == NULL) {
        return -1;
    }

    x = (PIT_SCREEN_WIDTH - banner->icon.width) / 2;
    y = (PIT_SCREEN_HEIGHT - banner->icon.height) / 2 - 16;

    return pit_blit_pixels(&screen->data[0][0], PIT_SCREEN_WIDTH, PIT_SCREEN_HEIGHT,
                           &banner->icon, x, y);
}
