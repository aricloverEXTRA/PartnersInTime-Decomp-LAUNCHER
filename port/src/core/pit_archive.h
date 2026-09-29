#ifndef PIT_CORE_PIT_ARCHIVE_H
#define PIT_CORE_PIT_ARCHIVE_H

#include <stddef.h>

/*
 * The outer `.dat` container used across the game's data files.
 *
 * Layout, as implemented by the decompilation's data tooling
 * (PiT tools/data_mod.py :: parse_offset_archive):
 *
 *   u32            table_size   always (count + 1) * 4
 *   u32            offsets[count + 1]
 *   u8             payload[]
 *
 * `offsets[0] == table_size`, so the payload starts immediately after the
 * table, and `offsets[count] == file size`, so the table spans the whole file.
 * Entry i is `payload[offsets[i] .. offsets[i + 1])`. A 0xFFFFFFFF word marks
 * the end of a partially used table. Empty entries are legal and are kept.
 *
 * The view borrows the caller's bytes; nothing is copied and no ROM data is
 * retained beyond the lifetime of the buffer passed to pit_archive_open.
 */

typedef struct {
    const unsigned char *data;
    size_t               size;
    unsigned int         count;      /* number of real entries */
    const unsigned char *table;      /* first offset word, little-endian u32s */
} pit_archive;

/* Returns 0 on success, -1 if the buffer is not a well-formed archive. */
int pit_archive_open(pit_archive *archive, const unsigned char *data, size_t size);

/*
 * Resolves one entry. Returns 0 on success with *out/*out_size set to a view
 * into the original buffer, or -1 if `index` is out of range. A zero-length
 * entry is valid and yields a non-NULL pointer with *out_size == 0.
 */
int pit_archive_entry(const pit_archive *archive, unsigned int index,
                      const unsigned char **out, size_t *out_size);

#endif
