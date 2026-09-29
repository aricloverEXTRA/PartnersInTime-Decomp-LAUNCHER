#include "core/pit_archive.h"

static unsigned int rd32(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

int pit_archive_open(pit_archive *archive, const unsigned char *data, size_t size)
{
    unsigned int table_size, count, i, previous;

    if (archive == NULL || data == NULL || size < 8) {
        return -1;
    }
    archive->data = data;
    archive->size = size;
    archive->count = 0;
    archive->table = data;

    table_size = rd32(data);
    if (table_size < 8 || (size_t)table_size > size || (table_size & 3u) != 0) {
        return -1;
    }
    count = table_size / 4u;

    /* A 0xFFFFFFFF word marks the end of a partially used table. */
    for (i = 0; i < count; i++) {
        if (rd32(data + (size_t)i * 4u) == 0xFFFFFFFFu) {
            count = i;
            break;
        }
    }
    if (count < 2) {
        return -1;
    }
    if (rd32(data) != table_size || rd32(data + (size_t)(count - 1) * 4u) != (unsigned int)size) {
        return -1;
    }

    /* Offsets must be non-decreasing and inside the buffer. */
    previous = 0;
    for (i = 0; i < count; i++) {
        unsigned int offset = rd32(data + (size_t)i * 4u);
        if (offset < previous || (size_t)offset > size) {
            return -1;
        }
        previous = offset;
    }

    archive->count = count - 1u;
    return 0;
}

int pit_archive_entry(const pit_archive *archive, unsigned int index,
                      const unsigned char **out, size_t *out_size)
{
    unsigned int start, end;

    if (archive == NULL || out == NULL || out_size == NULL || index >= archive->count) {
        return -1;
    }
    start = rd32(archive->table + (size_t)index * 4u);
    end = rd32(archive->table + (size_t)(index + 1u) * 4u);

    *out = archive->data + start;
    *out_size = (size_t)(end - start);
    return 0;
}
