#ifndef PIT_CORE_PIT_GFX_H
#define PIT_CORE_PIT_GFX_H

#include "core/pit.h"

/*
 * CPU-side image handling and tile decoding.
 *
 * This module knows about pixel formats only. It has no knowledge of any
 * Nintendo asset layout, contains no asset data, and never writes ROM bytes
 * anywhere. Formats are described here purely as publicly documented
 * encodings so that data read from the user's own ROM can be turned into
 * something the video path can display.
 *
 * Colour is carried as `pit_pixel` (see PIT_ARGB in core/pit.h) so decoded
 * images can be blitted straight into a `pit_screen` without conversion.
 */

typedef struct {
    int        width;
    int        height;
    pit_pixel *pixels;   /* row-major, width * height entries */
} pit_image;

/* Returns 0 on success, -1 on invalid size or allocation failure. */
int  pit_image_alloc(pit_image *image, int width, int height);
void pit_image_free(pit_image *image);

void pit_image_fill(pit_image *image, pit_pixel color);
pit_pixel pit_image_get(const pit_image *image, int x, int y);

/*
 * Expands a BGR555 colour to ARGB. This is the encoding the DS hardware uses
 * for 3D textures and for BG palettes; bit 15 is an alpha flag rather than a
 * colour bit, so it is honoured here instead of being treated as red.
 */
pit_pixel pit_bgr555(int alpha, unsigned int bgr555);

/* Alpha-blended blit with source alpha 0 treated as transparent. */
int pit_image_blit(pit_image *dst, const pit_image *src, int x, int y);

/*
 * Same as pit_image_blit but into a bare row-major pixel buffer. This is how a
 * decoded image reaches a fixed-size `pit_screen`, whose storage is a 2D array
 * rather than a pit_image and so must not be cast to one.
 */
int pit_blit_pixels(pit_pixel *dst, int dst_width, int dst_height,
                    const pit_image *src, int x, int y);

/* Nearest-neighbour integer upscale into a freshly allocated image. */
int pit_image_scale(pit_image *dst, const pit_image *src, int factor);

/*
 * Decodes a tiled bitmap into a new image.
 *
 * Tiles are 8x8 and stored row-major within each tile, which is the layout the
 * DS background engine and the cartridge banner both use:
 *
 *   4bpp: 32 bytes per tile, 4 bytes per row, two pixels per byte with the
 *         left pixel in the more significant bits.
 *   8bpp: 64 bytes per tile, 8 bytes per row, one pixel per byte.
 *
 * `palette` holds `palette_size` ARGB entries. `transparent_index` may be -1 to
 * keep every pixel opaque. Out-of-range palette indices are drawn as opaque
 * black so malformed data stays visible instead of reading adjacent memory.
 *
 * Returns 0 on success, -1 on invalid arguments or allocation failure.
 */
int pit_tiles_decode(pit_image *out, const unsigned char *tiles,
                     int bits_per_pixel, int tiles_x, int tiles_y,
                     const pit_pixel *palette, int palette_size,
                     int transparent_index);

/*
 * Decodes the DS OBJ 2bpp texture cells into a new image.
 *
 * This is a different encoding from the one above. Pixels are not read as a
 * packed byte stream: each cell is 8 pixels wide and 8, 12 or 16 pixels tall,
 * and its two index bits are taken from two interleaved bit planes. For column
 * `c` and row `r` of a cell, bit `4 * c + (r % 4)` of one 32-bit word supplies
 * the low index bit and the same bit of the next word supplies the high bit.
 * Every group of four rows therefore consumes one pair of words, so a cell is
 * `8 * cell_height / 4` bytes (16, 24 or 32).
 *
 * This is the layout the game expands into OBJ tile memory, where each row is
 * written 20 bytes apart; that padding is a property of the destination buffer
 * and is not present in the stored data, so cells here are read tightly packed.
 *
 * `cell_height` must be 8, 12 or 16. `cells_x` and `cells_y` describe the grid
 * of cells, giving a result `8 * cells_x` by `cell_height * cells_y` pixels.
 * Palette and transparency handling match pit_tiles_decode.
 *
 * Returns 0 on success, -1 on invalid arguments or allocation failure.
 */
int pit_tiles_decode_obj2bpp(pit_image *out, const unsigned char *cells,
                             int cell_height, int cells_x, int cells_y,
                             const pit_pixel *palette, int palette_size,
                             int transparent_index);

/* Packs one row of an image into RGBA8, 4 bytes per pixel. */
int pit_image_row_to_rgba8(const pit_image *image, int y, unsigned char *out);

/* Packs a whole image into row-major RGBA8, 4 bytes per pixel, no padding. */
int pit_image_to_rgba8(const pit_image *image, unsigned char *out);

#endif
