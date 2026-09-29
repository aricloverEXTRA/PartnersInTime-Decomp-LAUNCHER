#ifndef PIT_CORE_PIT_PNG_H
#define PIT_CORE_PIT_PNG_H

#include "core/pit_gfx.h"

/*
 * Minimal PNG writer.
 *
 * This exists so decoded output can be inspected outside the running program
 * without adding an image library to the build. It emits 8-bit RGBA with stored
 * (uncompressed) deflate blocks, which is a valid zlib stream and a valid PNG;
 * the files are simply larger than they would be with real compression.
 */

/* Writes `image` to `path` as PNG. Returns 0 on success, -1 on failure. */
int pit_png_write(const char *path, const pit_image *image);

#endif
