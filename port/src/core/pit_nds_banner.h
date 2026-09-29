#ifndef PIT_CORE_PIT_NDS_BANNER_H
#define PIT_CORE_PIT_NDS_BANNER_H

#include "core/pit_gfx.h"
#include "core/pit_rom.h"

/*
 * The cartridge banner block.
 *
 * This is the first graphic in the ROM that can be decoded end to end, which
 * makes it the natural first milestone for the renderer: it proves the
 * palette, the 4bpp tile path and the blit into a pit_screen all work against
 * real data before any of the game's own resource containers are involved.
 *
 * Layout (documented cartridge header format, not reverse engineered):
 *
 *   0x000  u16   version                 0x0001, 0x0002, 0x0003, 0x0103
 *   0x002  u16   crc16                   over 0x002..size
 *   0x004  u8    reserved[0x1C]
 *   0x020  u8    icon[512]               32x32, 4bpp, 4x4 tiles
 *   0x220  u16   palette[16]             BGR555
 *   0x240  u8    japanese_title[256]     Shift-JIS
 *   0x340  u8    english_title[256]      UTF-16LE
 *   0x440  u8    french_title[256]       UTF-16LE
 *   0x540  u8    german_title[256]       UTF-16LE
 *   0x640  u8    italian_title[256]      UTF-16LE
 *   0x740  u8    spanish_title[256]      UTF-16LE
 *
 * 0x004..0x01F is reserved and reads as zero, so the icon starts at 0x020
 * rather than 0x010. Reading from 0x010 yields a correct-looking image with a
 * blank strip, which is easy to mistake for real transparent artwork.
 *
 * The 4bpp icon and its palette sit at 0x020/0x220 in every version. Versions
 * 2 and 3 add an 8bpp icon and palette at 0x240/0x2E0 and move the titles to
 * 0x300, so only the title base changes between versions.
 */

#define PIT_BANNER_ICON_WIDTH   32
#define PIT_BANNER_ICON_HEIGHT  32
#define PIT_BANNER_ICON_TILES_X 4
#define PIT_BANNER_ICON_TILES_Y 4
#define PIT_BANNER_COLORS       16

typedef struct {
    unsigned int  version;
    unsigned int  crc16_stored;
    unsigned int  crc16_computed;
    int           crc_ok;
    int           crc_reproducible;
    unsigned int  offset;
    unsigned int  size;

    pit_pixel     palette[PIT_BANNER_COLORS];
    pit_image     icon;              /* 32x32 ARGB, index 0 transparent */
    pit_image     icon_scaled;       /* nearest-neighbour upscale for display */
    int           icon_scale;

    char          japanese_title[129];
    char          english_title[129];
    char          french_title[129];
    char          german_title[129];
    char          italian_title[129];
    char          spanish_title[129];
} pit_banner;

/*
 * Decodes the banner pointed to by `rom`. Returns 0 on success and fills
 * `banner`; the caller frees it with pit_banner_free. Returns -1 if the ROM has
 * no banner or the block is truncated.
 */
int  pit_banner_decode(pit_banner *banner, const pit_rom *rom);
void pit_banner_free(pit_banner *banner);

/* Draws the decoded icon centred on one of the frame's two screens. */
int  pit_banner_draw(const pit_banner *banner, pit_screen *screen);

/*
 * CRC-16/CCITT-FALSE over the banner body, matching the value the header
 * stores. Exposed so the diagnostic can report a mismatch instead of silently
 * rendering a block that failed its own checksum.
 *
 * Note that this does not reproduce the value in either supported ROM. A search
 * over every prefix length and the common CRC-16 variants (polynomials 0x1021,
 * 0x8005, 0xA001, 0x8408, 0x0589, 0x3D65, 0x8BB7, 0x1DCF against the usual
 * seeds, reflected and non-reflected) failed to reproduce either ROM's stored
 * value. The banner CRC is therefore a vendor-specific number rather than a
 * data-integrity check, and the port reports it without gating rendering on it.
 */
unsigned int pit_banner_crc16(const unsigned char *data, size_t len);

#endif
