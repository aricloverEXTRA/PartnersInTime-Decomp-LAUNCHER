#include "core/pit_rom.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int rd32(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

static unsigned int rd16(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static int in_range(const pit_rom *rom, unsigned int off, unsigned int len)
{
    if (off > rom->size || len > rom->size) {
        return 0;
    }
    return (size_t)off + (size_t)len <= rom->size;
}

int pit_rom_open(pit_rom *rom, const char *path)
{
    FILE *fp;
    long size;
    const unsigned char *h;

    if (rom == NULL || path == NULL) {
        return -1;
    }
    memset(rom, 0, sizeof(*rom));

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return -1;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return -1;
    }
    size = ftell(fp);
    if (size <= 0) {
        fclose(fp);
        return -1;
    }
    rewind(fp);

    rom->data = (unsigned char *)malloc((size_t)size);
    if (rom->data == NULL) {
        fclose(fp);
        return -1;
    }
    if (fread(rom->data, 1, (size_t)size, fp) != (size_t)size) {
        fclose(fp);
        free(rom->data);
        rom->data = NULL;
        return -1;
    }
    fclose(fp);
    rom->size = (size_t)size;

    if (rom->size < 0x200) {
        goto fail;
    }
    h = rom->data;

    memcpy(rom->title, h + 0x00, 12);
    rom->title[12] = '\0';
    memcpy(rom->game_code, h + 0x0C, 4);
    rom->game_code[4] = '\0';
    memcpy(rom->maker_code, h + 0x10, 2);
    rom->maker_code[2] = '\0';
    rom->unit_code = h[0x12];

    rom->arm9_offset    = rd32(h + 0x20);
    rom->arm9_entry     = rd32(h + 0x24);
    rom->arm9_load      = rd32(h + 0x28);
    rom->arm9_size      = rd32(h + 0x2C);
    rom->arm7_offset    = rd32(h + 0x30);
    rom->arm7_entry     = rd32(h + 0x34);
    rom->arm7_load      = rd32(h + 0x38);
    rom->arm7_size      = rd32(h + 0x3C);
    rom->fnt_offset     = rd32(h + 0x40);
    rom->fnt_size       = rd32(h + 0x44);
    rom->fat_offset     = rd32(h + 0x48);
    rom->fat_size       = rd32(h + 0x4C);
    rom->banner_offset  = rd32(h + 0x68);

    /* Banner size is fixed by the banner's own version field, not the header. */
    if (in_range(rom, rom->banner_offset, 0x20)) {
        const unsigned char *b = rom->data + rom->banner_offset;
        switch (rd16(b)) {
        case 0x0001: rom->banner_size = 0x0840; break;
        case 0x0002: rom->banner_size = 0x0940; break;
        case 0x0003: rom->banner_size = 0x0A40; break;
        case 0x0103: rom->banner_size = 0x23C0; break;
        default:     rom->banner_size = 0; break;
        }
    }

    /* The FAT's leading entries describe the ARM binaries, not archive files. */
    if (rom->fat_size < 8 || (rom->fat_size % 8) != 0) {
        goto fail;
    }
    rom->file_count = rom->fat_size / 8;

    if (!in_range(rom, rom->fnt_offset, rom->fnt_size)) {
        goto fail;
    }
    if (!in_range(rom, rom->fat_offset, rom->fat_size)) {
        goto fail;
    }

    /*
     * Root directory record: {u32 subtable_offset, u16 first_file_id, u16 pad}.
     * The first archive file id comes from the FNT, not the FAT.
     */
    if (rom->fnt_size < 0x40) {
        goto fail;
    }
    rom->first_file_id = rd16(rom->data + rom->fnt_offset + 4);

    return 0;

fail:
    free(rom->data);
    rom->data = NULL;
    rom->size = 0;
    return -1;
}

void pit_rom_close(pit_rom *rom)
{
    if (rom == NULL) {
        return;
    }
    free(rom->data);
    rom->data = NULL;
    rom->size = 0;
}

/*
 * The FNT begins with a directory table of 8-byte records
 * ({u32 subtable_offset, u16 first_file_id, u16 parent_id}), which runs until
 * the first subtable begins.
 *
 * Each subtable entry packs its kind and name length into one byte (bit 7 set
 * = directory, bits 0-6 = length), then the raw name bytes. Directory entries
 * are followed by a u16 holding 0xF000 | dir_id; file entries have no id field
 * at all and take the next implicit id, counting up from the directory's
 * first_file_id. A zero byte ends the subtable. Names are Shift-JIS and are
 * copied through as raw bytes.
 */
#define PIT_FNT_DIR_BIT   0x80
#define PIT_FNT_NAME_MASK 0x7F
#define PIT_FNT_DIR_ID    0xF000

static int walk_dir(const pit_rom *rom, unsigned int dir_id, unsigned int depth,
                    const char *prefix, pit_fs_entry *out, int max_entries,
                    int *count)
{
    const unsigned char *fnt = rom->data + rom->fnt_offset;
    const unsigned char *rec;
    unsigned int sub_off, next_id, cursor;
    /* Paths are built in a buffer with room for a separator and a terminator,
     * so appending one name can never overrun it. */
    char name[256];
    char child_prefix[sizeof(name) + 2];

    /*
     * The directory table is not capped at eight entries; it runs until the
     * first subtable begins. Validate the record is inside the FNT and that its
     * subtable offset is plausible, which rejects the subtable bytes that
     * follow the table.
     */
    if (depth > 32 || (size_t)dir_id * 8 + 8 > rom->fnt_size) {
        return -1;
    }
    rec = fnt + dir_id * 8;

    sub_off = rd32(rec);
    next_id = rd16(rec + 4);

    if (sub_off == 0 || (size_t)sub_off >= rom->fnt_size) {
        return -1;
    }
    cursor = sub_off;

    for (;;) {
        unsigned int flags, name_len, raw_len;
        const unsigned char *namep;

        if ((size_t)cursor >= rom->fnt_size) {
            return -1;
        }
        flags = fnt[cursor];
        if (flags == 0) {
            break;
        }
        raw_len = flags & PIT_FNT_NAME_MASK;
        /* Only directory entries carry a trailing u16 id. */
        if ((size_t)cursor + 1 + raw_len + ((flags & PIT_FNT_DIR_BIT) ? 2u : 0u)
            > rom->fnt_size) {
            return -1;
        }

        namep = fnt + cursor + 1;
        name_len = raw_len;
        if (name_len >= sizeof(name)) {
            name_len = sizeof(name) - 1;
        }
        memcpy(name, namep, name_len);
        name[name_len] = '\0';

        if (flags & PIT_FNT_DIR_BIT) {
            unsigned int id = rd16(namep + raw_len);
            cursor += 1 + raw_len + 2;

            if ((id & 0xF000) != PIT_FNT_DIR_ID) {
                return -1;
            }
            snprintf(child_prefix, sizeof(child_prefix), "%s%s/", prefix, name);
            if (walk_dir(rom, id & 0x0FFF, depth + 1, child_prefix, out,
                         max_entries, count) != 0) {
                return -1;
            }
        } else {
            pit_fs_entry *e;
            cursor += 1 + raw_len;

            if (next_id >= rom->file_count) {
                continue;
            }
            if (*count >= max_entries) {
                return 0;
            }
            e = &out[*count];
            memset(e, 0, sizeof(*e));
            e->id = next_id++;
            e->parent = dir_id;
            e->is_dir = 0;
            e->offset = rd32(rom->data + rom->fat_offset + e->id * 8);
            e->size = rd32(rom->data + rom->fat_offset + e->id * 8 + 4) - e->offset;
            snprintf(e->path, sizeof(e->path), "%s%s", prefix, name);
            (*count)++;
        }
    }

    return 0;
}

int pit_rom_list(const pit_rom *rom, unsigned int dir_id, pit_fs_entry *out,
                 int max_entries)
{
    int count = 0;

    if (rom == NULL || rom->data == NULL || out == NULL || max_entries <= 0) {
        return -1;
    }
    if (walk_dir(rom, dir_id, 0, "", out, max_entries, &count) != 0) {
        return -1;
    }
    return count;
}

const pit_fs_entry *pit_rom_find(const pit_rom *rom, const char *path)
{
    static pit_fs_entry entries[2048];
    int n, i;

    if (rom == NULL || path == NULL) {
        return NULL;
    }
    n = pit_rom_list(rom, 0, entries, 2048);
    if (n <= 0) {
        return NULL;
    }
    for (i = 0; i < n; i++) {
        if (strcmp(entries[i].path, path) == 0) {
            return &entries[i];
        }
    }
    return NULL;
}

void *pit_rom_read(const pit_rom *rom, const char *path, size_t *out_size)
{
    const pit_fs_entry *e = pit_rom_find(rom, path);
    unsigned char *buf;

    if (e == NULL || e->is_dir) {
        return NULL;
    }
    if (!in_range(rom, e->offset, e->size)) {
        return NULL;
    }
    buf = (unsigned char *)malloc(e->size ? e->size : 1);
    if (buf == NULL) {
        return NULL;
    }
    memcpy(buf, rom->data + e->offset, e->size);
    if (out_size != NULL) {
        *out_size = e->size;
    }
    return buf;
}
