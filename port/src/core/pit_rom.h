#ifndef PIT_CORE_PIT_ROM_H
#define PIT_CORE_PIT_ROM_H

#include <stddef.h>

/*
 * Runtime access to a user-supplied NDS cartridge image.
 *
 * Nothing here contains Nintendo assets. The ROM stays on the user's disk and
 * is read in place; the port never extracts it to disk and never writes any of
 * its contents into the executable.
 */

typedef struct {
    unsigned char *data;
    size_t         size;

    char           title[13];
    char           game_code[5];
    char           maker_code[3];
    unsigned char  unit_code;

    unsigned int   arm9_offset;     /* file offset of the ARM9 binary */
    unsigned int   arm9_entry;      /* ARM9 entry address */
    unsigned int   arm9_load;       /* ARM9 load address */
    unsigned int   arm9_size;
    unsigned int   arm7_offset;
    unsigned int   arm7_entry;
    unsigned int   arm7_load;
    unsigned int   arm7_size;
    unsigned int   fnt_offset;
    unsigned int   fnt_size;
    unsigned int   fat_offset;
    unsigned int   fat_size;
    unsigned int   banner_offset;   /* icon/banner, 0 when the ROM has none */
    unsigned int   banner_size;

    unsigned int   first_file_id;   /* files before the NitroFS root */
    unsigned int   file_count;      /* entries in the FAT */
} pit_rom;

int  pit_rom_open(pit_rom *rom, const char *path);
void pit_rom_close(pit_rom *rom);

/* ---------------------------------------------------------------- NitroFS */

typedef struct {
    unsigned int id;        /* index into the FAT */
    unsigned int parent;    /* containing directory id */
    int          is_dir;
    unsigned int offset;    /* absolute file offset (files only) */
    unsigned int size;      /* file size in bytes (files only) */
    char         path[256]; /* full path inside the archive */
} pit_fs_entry;

/* Directory listing. Returns the number of entries written, or -1 on error. */
int pit_rom_list(const pit_rom *rom, unsigned int dir_id, pit_fs_entry *out,
                 int max_entries);

const pit_fs_entry *pit_rom_find(const pit_rom *rom, const char *path);

/*
 * Reads a whole file out of the archive. Returns a malloc'd buffer the caller
 * frees, or NULL if the path is missing or is a directory. *out_size receives
 * the byte count.
 */
void *pit_rom_read(const pit_rom *rom, const char *path, size_t *out_size);

#endif
