#include "core/pit_gfx.h"

#include <stdlib.h>
#include <string.h>

#define TILE_DIM 8
#define OBJ_CELL_WIDTH 8

int pit_image_alloc(pit_image *image, int width, int height)
{
    size_t count;

    if (image == NULL || width <= 0 || height <= 0) {
        return -1;
    }

    count = (size_t)width * (size_t)height;
    image->width = width;
    image->height = height;
    image->pixels = (pit_pixel *)calloc(count, sizeof(pit_pixel));
    if (image->pixels == NULL) {
        image->width = 0;
        image->height = 0;
        return -1;
    }
    return 0;
}

void pit_image_free(pit_image *image)
{
    if (image == NULL) {
        return;
    }
    free(image->pixels);
    image->pixels = NULL;
    image->width = 0;
    image->height = 0;
}

void pit_image_fill(pit_image *image, pit_pixel color)
{
    int total, i;

    if (image == NULL || image->pixels == NULL) {
        return;
    }
    total = image->width * image->height;
    for (i = 0; i < total; i++) {
        image->pixels[i] = color;
    }
}

pit_pixel pit_image_get(const pit_image *image, int x, int y)
{
    if (image == NULL || image->pixels == NULL) {
        return 0;
    }
    if (x < 0 || y < 0 || x >= image->width || y >= image->height) {
        return 0;
    }
    return image->pixels[y * image->width + x];
}

pit_pixel pit_bgr555(int alpha, unsigned int bgr555)
{
    int r, g, b;

    r = (int)(bgr555 & 0x1Fu);
    g = (int)((bgr555 >> 5) & 0x1Fu);
    b = (int)((bgr555 >> 10) & 0x1Fu);

    return PIT_ARGB(alpha, r * 255 / 31, g * 255 / 31, b * 255 / 31);
}

int pit_blit_pixels(pit_pixel *dst, int dst_width, int dst_height,
                    const pit_image *src, int x, int y)
{
    int sx, sy, dx, dy;

    if (dst == NULL || src == NULL || src->pixels == NULL) {
        return -1;
    }

    for (sy = 0; sy < src->height; sy++) {
        dy = y + sy;
        if (dy < 0 || dy >= dst_height) {
            continue;
        }
        for (sx = 0; sx < src->width; sx++) {
            pit_pixel c = src->pixels[sy * src->width + sx];
            int sa, r, g, b;

            dx = x + sx;
            if (dx < 0 || dx >= dst_width) {
                continue;
            }
            if ((c >> 24) == 0u) {
                continue;
            }

            sa = (int)(c >> 24) & 0xFF;
            r = (int)(c >> 16) & 0xFF;
            g = (int)(c >> 8) & 0xFF;
            b = (int)(c) & 0xFF;

            if (sa == 255) {
                dst[dy * dst_width + dx] = c;
            } else {
                pit_pixel under = dst[dy * dst_width + dx];
                int ua = (int)(under >> 24) & 0xFF;
                int ur = (int)(under >> 16) & 0xFF;
                int ug = (int)(under >> 8) & 0xFF;
                int ub = (int)(under) & 0xFF;
                int inv = 255 - sa;
                int out_a = sa * 255 + ua * inv;
                int nr, ng, nb;

                if (out_a <= 0) {
                    dst[dy * dst_width + dx] = 0;
                    continue;
                }

                nr = (r * sa * 255 + ur * ua * inv) / out_a;
                ng = (g * sa * 255 + ug * ua * inv) / out_a;
                nb = (b * sa * 255 + ub * ua * inv) / out_a;

                dst[dy * dst_width + dx] = PIT_ARGB((out_a + 127) / 255, nr, ng, nb);
            }
        }
    }
    return 0;
}

int pit_image_blit(pit_image *dst, const pit_image *src, int x, int y)
{
    if (dst == NULL || dst->pixels == NULL) {
        return -1;
    }
    return pit_blit_pixels(dst->pixels, dst->width, dst->height, src, x, y);
}

int pit_image_scale(pit_image *dst, const pit_image *src, int factor)
{
    int x, y;

    if (dst == NULL || src == NULL || src->pixels == NULL || factor < 1) {
        return -1;
    }
    if (pit_image_alloc(dst, src->width * factor, src->height * factor) != 0) {
        return -1;
    }
    for (y = 0; y < dst->height; y++) {
        for (x = 0; x < dst->width; x++) {
            dst->pixels[y * dst->width + x] = src->pixels[(y / factor) * src->width + (x / factor)];
        }
    }
    return 0;
}

int pit_tiles_decode(pit_image *out, const unsigned char *tiles,
                     int bits_per_pixel, int tiles_x, int tiles_y,
                     const pit_pixel *palette, int palette_size,
                     int transparent_index)
{
    int tile_bytes, tx, ty, px, py, index;
    const unsigned char *tile;

    if (out == NULL || tiles == NULL || palette == NULL) {
        return -1;
    }
    if (bits_per_pixel != 4 && bits_per_pixel != 8) {
        return -1;
    }
    if (tiles_x <= 0 || tiles_y <= 0 || palette_size <= 0) {
        return -1;
    }

    /*
     * 4bpp is half a byte per pixel, so the tile size has to be computed as
     * total_bits / 8. Deriving a byte count per pixel first and dividing that
     * truncates to zero for 4bpp, which collapses every tile onto the same
     * address and renders each 8-pixel band repeated across the image.
     */
    tile_bytes = TILE_DIM * TILE_DIM * bits_per_pixel / 8;

    if (pit_image_alloc(out, tiles_x * TILE_DIM, tiles_y * TILE_DIM) != 0) {
        return -1;
    }

    for (ty = 0; ty < tiles_y; ty++) {
        for (tx = 0; tx < tiles_x; tx++) {
            tile = tiles + ((size_t)ty * (size_t)tiles_x + (size_t)tx) * (size_t)tile_bytes;

            for (py = 0; py < TILE_DIM; py++) {
                for (px = 0; px < TILE_DIM; px++) {
                    if (bits_per_pixel == 4) {
                        unsigned char byte = tile[py * 4 + px / 2];
                        index = (px & 1) ? (byte & 0x0F) : (byte >> 4);
                    } else {
                        index = tile[py * 8 + px];
                    }

                    {
                        pit_pixel c;
                        int out_x = tx * TILE_DIM + px;
                        int out_y = ty * TILE_DIM + py;

                        if (index < palette_size) {
                            c = palette[index];
                        } else {
                            c = PIT_ARGB(255, 0, 0, 0);
                        }

                        if (index == transparent_index) {
                            c = 0;
                        }

                        out->pixels[out_y * out->width + out_x] = c;
                    }
                }
            }
        }
    }

    return 0;
}

/* Reads one little-endian plane word out of the cell stream. */
static unsigned int obj2bpp_plane(const unsigned char *cells, size_t word)
{
    return (unsigned int)cells[word * 4u + 0u] |
           ((unsigned int)cells[word * 4u + 1u] << 8) |
           ((unsigned int)cells[word * 4u + 2u] << 16) |
           ((unsigned int)cells[word * 4u + 3u] << 24);
}

int pit_tiles_decode_obj2bpp(pit_image *out, const unsigned char *cells,
                             int cell_height, int cells_x, int cells_y,
                             const pit_pixel *palette, int palette_size,
                             int transparent_index)
{
    int cell_bytes, cx, cy, x, y;
    size_t cell;

    if (out == NULL || cells == NULL || palette == NULL) {
        return -1;
    }
    if (cell_height != 8 && cell_height != 12 && cell_height != 16) {
        return -1;
    }
    if (cells_x <= 0 || cells_y <= 0 || palette_size <= 0) {
        return -1;
    }

    cell_bytes = OBJ_CELL_WIDTH * cell_height / 4;

    if (pit_image_alloc(out, cells_x * OBJ_CELL_WIDTH, cells_y * cell_height) != 0) {
        return -1;
    }

    for (cy = 0; cy < cells_y; cy++) {
        for (cx = 0; cx < cells_x; cx++) {
            size_t first_word;

            cell = ((size_t)cy * (size_t)cells_x + (size_t)cx) * (size_t)cell_bytes;
            first_word = cell / 4u;

            for (y = 0; y < cell_height; y++) {
                unsigned int low = obj2bpp_plane(cells, first_word + (size_t)(y / 4) * 2u);
                unsigned int high = obj2bpp_plane(cells, first_word + (size_t)(y / 4) * 2u + 1u);

                for (x = 0; x < OBJ_CELL_WIDTH; x++) {
                    int bit = 4 * x + (y % 4);
                    int index = (int)((low >> bit) & 1u) + 2 * (int)((high >> bit) & 1u);
                    pit_pixel c;
                    int out_x = cx * OBJ_CELL_WIDTH + x;
                    int out_y = cy * cell_height + y;

                    if (index < palette_size) {
                        c = palette[index];
                    } else {
                        c = PIT_ARGB(255, 0, 0, 0);
                    }

                    if (index == transparent_index) {
                        c = 0;
                    }

                    out->pixels[out_y * out->width + out_x] = c;
                }
            }
        }
    }

    return 0;
}

int pit_image_row_to_rgba8(const pit_image *image, int y, unsigned char *out)
{
    int x;

    if (image == NULL || image->pixels == NULL || out == NULL) {
        return -1;
    }
    if (y < 0 || y >= image->height) {
        return -1;
    }

    for (x = 0; x < image->width; x++) {
        pit_pixel c = image->pixels[y * image->width + x];
        out[x * 4 + 0] = (unsigned char)((c >> 16) & 0xFF);
        out[x * 4 + 1] = (unsigned char)((c >> 8) & 0xFF);
        out[x * 4 + 2] = (unsigned char)((c) & 0xFF);
        out[x * 4 + 3] = (unsigned char)((c >> 24) & 0xFF);
    }
    return 0;
}

int pit_image_to_rgba8(const pit_image *image, unsigned char *out)
{
    int y, stride;

    if (image == NULL || out == NULL) {
        return -1;
    }

    stride = image->width * 4;
    for (y = 0; y < image->height; y++) {
        if (pit_image_row_to_rgba8(image, y, out + (size_t)y * (size_t)stride) != 0) {
            return -1;
        }
    }
    return 0;
}
