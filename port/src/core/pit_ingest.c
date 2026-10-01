/*
 * Generated-data export for the PiT decompilation.
 *
 * The whole export is derived from the user's own cartridge bytes: the banner
 * PNG is re-encoded from the ROM's banner block, the NitroFS tree is a listing
 * of the ROM's directory, the SDAT probe reads the head of the user's sound
 * bank, and the manifest records what was produced. Nothing is compared against
 * stored data, downloaded, or written back into the ROM.
 *
 * The SDAT header layout follows the public cartridge sound-bank format
 * (magic "SDAT", BOM 0xFFFE, version 0x0100 for 1.0, six u32 section offsets).
 * The EUR release's sound_data.sdat holds four sections (SYMB, PINF, FINF,
 * PBLK) and its banner carries the embedded title captured below the icon.
 */

#include "core/pit_ingest.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/pit.h"
#include "core/pit_nds_banner.h"
#include "core/pit_patch_data.h"
#include "core/pit_png.h"
#include "core/pit_sha1.h"

#if defined(_WIN32)
#include <direct.h>
#define PIT_MKDIR(p) (_mkdir((p)))
#else
#include <sys/stat.h>
#define PIT_MKDIR(p) (mkdir((p), 0755))
#endif

#define INGEST_CARD_W     256
#define INGEST_CARD_H     128
#define INGEST_MAX_FILES  2048
#define INGEST_TITLE_CAP  14
#define INGEST_SOUND_PATH "Sound/sound_data.sdat"

#define INK_BG    PIT_ARGB(255, 0x00, 0x10, 0x29)
#define INK_GOLD  PIT_ARGB(255, 0xDE, 0x94, 0x29)
#define INK_TEXT  PIT_ARGB(255, 0xE9, 0xEE, 0xF6)

/* ------------------------------------------------------------------ helpers */

static void ing_log(pit_patch_log_fn log, void *ctx, const char *message)
{
    if (log) {
        log(ctx, PIT_PATCH_STEPS, message, 0.98);
    }
}

static void hex_digest(const unsigned char *digest, char *out)
{
    static const char digits[] = "0123456789abcdef";
    int i;

    for (i = 0; i < PIT_SHA1_DIGEST_LEN; i++) {
        out[i * 2] = digits[(digest[i] >> 4) & 0x0F];
        out[i * 2 + 1] = digits[digest[i] & 0x0F];
    }
    out[PIT_SHA1_HEX_LEN - 1] = '\0';
}

static void compute_sha1(const pit_rom *rom, char out[PIT_SHA1_HEX_LEN])
{
    pit_sha1_ctx ctx;
    unsigned char digest[PIT_SHA1_DIGEST_LEN];

    pit_sha1_init(&ctx);
    pit_sha1_update(&ctx, rom->data, rom->size);
    pit_sha1_final(&ctx, digest);
    hex_digest(digest, out);
}

static int make_dirs(const char *path)
{
    char buf[1024];
    size_t len = strlen(path);
    size_t i;

    if (path[0] == '\0' || len >= sizeof(buf)) {
        return -1;
    }
    memcpy(buf, path, len + 1);

    for (i = 1; i < len; i++) {
        if (buf[i] == '/' || buf[i] == '\\') {
            buf[i] = '\0';
            if (buf[0] != '\0' && PIT_MKDIR(buf) != 0 && errno != EEXIST) {
                return -1;
            }
            buf[i] = (char)((buf[i] == '/') ? '/' : '\\');
        }
    }
    if (PIT_MKDIR(buf) != 0 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

static int join_path(char *out, size_t out_size, const char *dir,
                     const char *name)
{
    size_t dl = strlen(dir);
    size_t nl = strlen(name);
    int need_sep = dl > 0 && dir[dl - 1] != '/' && dir[dl - 1] != '\\';

    if (dl + (need_sep ? 1u : 0u) + nl + 1u > out_size) {
        return -1;
    }
    memcpy(out, dir, dl);
    if (need_sep) {
        out[dl++] = '/';
    }
    memcpy(out + dl, name, nl + 1u);
    return 0;
}

static int copy_text(const char *text, char *out, size_t out_size)
{
    size_t n = strlen(text);

    if (n >= out_size) {
        n = out_size - 1;
    }
    memcpy(out, text, n);
    out[n] = '\0';
    return (n == strlen(text)) ? 0 : -1;
}

/* ------------------------------------------------------- banner.png drawing */

static void img_put(pit_image *img, int x, int y, pit_pixel color)
{
    if (x >= 0 && x < img->width && y >= 0 && y < img->height) {
        img->pixels[y * img->width + x] = color;
    }
}

static void img_rect(pit_image *img, int x, int y, int w, int h,
                     pit_pixel color)
{
    int i, j;

    for (j = 0; j < h; j++) {
        for (i = 0; i < w; i++) {
            img_put(img, x + i, y + j, color);
        }
    }
}

static void img_text(pit_image *img, int x, int y, pit_pixel color,
                     const char *text, int max_chars)
{
    int n = 0;

    while (*text != '\0' && n < max_chars) {
        unsigned int glyph = (unsigned int)(unsigned char)*text;

        if (glyph >= 0x20u && glyph < 0x20u + PIT_FONT_GLYPH_COUNT) {
            const unsigned char *pix = PIT_FONT8X8[glyph - 0x20u];
            int row, col;

            for (row = 0; row < 8; row++) {
                for (col = 0; col < 5; col++) {
                    if (pix[row] & (0x80u >> col)) {
                        img_put(img, x + col, y + row, color);
                    }
                }
            }
        }
        x += 6;
        text++;
        n++;
    }
}

static int export_banner(const pit_rom *rom, const char *export_dir)
{
    pit_banner banner;
    pit_image card;
    char path[1024];
    char title[INGEST_TITLE_CAP + 1];
    char crc_line[32];
    int i;
    int rc;

    if (pit_banner_decode(&banner, rom) != 0) {
        return -1;
    }

    if (copy_text(banner.english_title, title, sizeof(title)) != 0) {
        title[INGEST_TITLE_CAP] = '\0';
    }

    rc = pit_image_alloc(&card, INGEST_CARD_W, INGEST_CARD_H);
    if (rc != 0) {
        pit_banner_free(&banner);
        return -1;
    }
    pit_image_fill(&card, INK_BG);
    pit_image_blit(&card, &banner.icon_scaled, 0, 0);

    img_text(&card, 140, 8, INK_GOLD, "CARTRIDGE BANNER", 19);
    img_text(&card, 140, 20, INK_TEXT, title, INGEST_TITLE_CAP);
    if (banner.crc_reproducible && banner.crc_ok) {
        snprintf(crc_line, sizeof(crc_line), "CRC-16 %04X OK", banner.crc16_stored);
    } else {
        snprintf(crc_line, sizeof(crc_line), "CRC-16 %04X", banner.crc16_stored);
    }
    img_text(&card, 140, 32, INK_TEXT, crc_line, 19);
    img_text(&card, 140, 44, INK_GOLD, "PALETTE (16)", 19);
    for (i = 0; i < PIT_BANNER_COLORS; i++) {
        int col = i % 8;
        int row = i / 8;

        img_rect(&card, 140 + col * 8, 54 + row * 14, 6, 12, banner.palette[i]);
    }
    img_text(&card, 140, 88, INK_GOLD, "ICON 32X32 4BPP", 19);

    rc = -1;
    if (join_path(path, sizeof(path), export_dir, "banner.png") == 0) {
        rc = pit_png_write(path, &card);
    }
    pit_image_free(&card);
    pit_banner_free(&banner);
    return rc;
}

/* -------------------------------------------------------- nitrofs.txt export */

static int export_nitrofs(const pit_rom *rom, const char *export_dir,
                          int *file_count)
{
    pit_fs_entry *entries;
    char path[1024];
    FILE *fp;
    int count;
    int i;

    entries = (pit_fs_entry *)malloc(sizeof(*entries) * INGEST_MAX_FILES);
    if (entries == NULL) {
        return -1;
    }
    count = pit_rom_list(rom, 0, entries, INGEST_MAX_FILES);
    if (count < 0) {
        free(entries);
        return -1;
    }
    if (join_path(path, sizeof(path), export_dir, "nitrofs.txt") != 0) {
        free(entries);
        return -1;
    }
    fp = fopen(path, "wb");
    if (fp == NULL) {
        free(entries);
        return -1;
    }

    fprintf(fp, "# NitroFS of the source cartridge: %s (%s), %d files.\n",
            rom->title, rom->game_code, count);
    fprintf(fp, "# %5s  %10s  %10s  %s\n", "id", "offset", "size", "path");
    for (i = 0; i < count; i++) {
        const pit_fs_entry *e = &entries[i];

        fprintf(fp, "  %5u  %08X  %10u  %s\n", e->id, e->offset, e->size,
                e->path);
    }
    if (fclose(fp) != 0) {
        free(entries);
        return -1;
    }
    free(entries);
    if (file_count) {
        *file_count = count;
    }
    return 0;
}

/* ------------------------------------------------- sound_data.sdat.txt export */

typedef struct {
    int            found;
    int            good;
    unsigned int   version_major;
    unsigned int   version_minor;
    unsigned int   sections;
} sdat_probe;

static void probe_sdat(const unsigned char *buf, size_t size, sdat_probe *out)
{
    unsigned int bom, version;

    memset(out, 0, sizeof(*out));
    if (buf == NULL || size < 16u) {
        out->found = (buf != NULL);
        return;
    }
    if (memcmp(buf, "SDAT", 4) != 0) {
        out->found = 1;
        return;
    }
    out->found = 1;
    bom = (unsigned int)buf[4] | ((unsigned int)buf[5] << 8);
    version = (unsigned int)buf[6] | ((unsigned int)buf[7] << 8);
    if (bom != 0xFEFFu) {
        return;
    }
    out->good = 1;
    out->version_major = (version >> 8) & 0xFFu;
    out->version_minor = version & 0xFFu;
    out->sections = (unsigned int)buf[0x0E] | ((unsigned int)buf[0x0F] << 8);
}

static int export_sdat(const pit_rom *rom, const char *export_dir,
                       sdat_probe *probe)
{
    unsigned char *buf;
    size_t size = 0;
    char path[1024];
    FILE *fp;
    int i;

    buf = (unsigned char *)pit_rom_read(rom, INGEST_SOUND_PATH, &size);
    probe_sdat(buf, size, probe);

    if (join_path(path, sizeof(path), export_dir,
                  "sound_data.sdat.txt") != 0) {
        free(buf);
        return -1;
    }
    fp = fopen(path, "wb");
    if (fp == NULL) {
        free(buf);
        return -1;
    }

    if (!probe->found) {
        fprintf(fp, "sound_data.sdat: not present in this ROM.\r\n");
    } else if (size < 16u) {
        fprintf(fp, "sound_data.sdat: truncated (%u bytes).\r\n",
                (unsigned int)size);
    } else if (memcmp(buf, "SDAT", 4) != 0) {
        fprintf(fp, "sound_data.sdat: not an SDAT stream (magic %c%c%c%c).\r\n",
                buf[0], buf[1], buf[2], buf[3]);
    } else {
        unsigned int bom = (unsigned int)buf[4] | ((unsigned int)buf[5] << 8);
        unsigned int version = (unsigned int)buf[6] | ((unsigned int)buf[7] << 8);

        fprintf(fp, "sound_data.sdat: SDAT header\r\n");
        fprintf(fp, "  magic        : %c%c%c%c\r\n", buf[0], buf[1], buf[2],
                buf[3]);
        fprintf(fp, "  byte order   : 0x%04X (%s)\r\n", bom,
                (bom == 0xFEFFu) ? "little-endian" : "other");
        fprintf(fp, "  version      : v%u.%u\r\n", (version >> 8) & 0xFFu,
                version & 0xFFu);
        if (size >= 12u) {
            fprintf(fp, "  file size    : %u (0x%08X)\r\n",
                    (unsigned int)((unsigned int)buf[8] |
                                   ((unsigned int)buf[9] << 8) |
                                   ((unsigned int)buf[10] << 16) |
                                   ((unsigned int)buf[11] << 24)),
                    (unsigned int)((unsigned int)buf[8] |
                                   ((unsigned int)buf[9] << 8) |
                                   ((unsigned int)buf[10] << 16) |
                                   ((unsigned int)buf[11] << 24)));
        }
        if (size >= 16u) {
            unsigned int sections = (unsigned int)buf[0x0E] |
                                    ((unsigned int)buf[0x0F] << 8);

            fprintf(fp, "  header size  : 0x%04X\r\n",
                    (unsigned int)buf[0x0C] | ((unsigned int)buf[0x0D] << 8));
            fprintf(fp, "  section count: %u\r\n", sections);
            fprintf(fp, "  sections:\r\n");
            for (i = 0; i < 6 && 0x10u + (unsigned int)i * 4u + 4u <= size; i++) {
                unsigned int off = (unsigned int)buf[0x10 + i * 4] |
                                   ((unsigned int)buf[0x11 + i * 4] << 8) |
                                   ((unsigned int)buf[0x12 + i * 4] << 16) |
                                   ((unsigned int)buf[0x13 + i * 4] << 24);

                fprintf(fp, "    [%d] 0x%08X\r\n", i, off);
            }
        }
    }
    if (fclose(fp) != 0) {
        free(buf);
        return -1;
    }
    free(buf);
    return 0;
}

/* ------------------------------------------------------ manifest.json export */

static void json_write(FILE *fp, const char *text)
{
    const unsigned char *p = (const unsigned char *)text;

    fputc('"', fp);
    for (; *p != '\0'; p++) {
        switch (*p) {
        case '"':  fputs("\\\"", fp); break;
        case '\\': fputs("\\\\", fp); break;
        case '\b': fputs("\\b", fp); break;
        case '\f': fputs("\\f", fp); break;
        case '\n': fputs("\\n", fp); break;
        case '\r': fputs("\\r", fp); break;
        case '\t': fputs("\\t", fp); break;
        default:
            if (*p < 0x20u) {
                fprintf(fp, "\\u%04X", *p);
            } else {
                fputc(*p, fp);
            }
            break;
        }
    }
    fputc('"', fp);
}

static int export_manifest(const pit_rom *rom, const char *export_dir,
                           const pit_patch_info *info, const char *sha1,
                           int file_count, int sdat_found)
{
    char path[1024];
    FILE *fp;
    unsigned int target_offset = 0;

    if (info && info->target_offset != 0) {
        target_offset = info->target_offset;
    } else {
        const pit_fs_entry *target = pit_rom_find(rom, PIT_TARGET_PATH);

        if (target) {
            target_offset = target->offset;
        }
    }

    if (join_path(path, sizeof(path), export_dir, "manifest.json") != 0) {
        return -1;
    }
    fp = fopen(path, "wb");
    if (fp == NULL) {
        return -1;
    }

    fprintf(fp, "{\n");
    fprintf(fp, "  \"format\": \"pit-ingest\",\n");
    fprintf(fp, "  \"format_version\": 1,\n");
    fprintf(fp, "  \"generated_by\": \"PiT Launcher\",\n");
    fprintf(fp, "  \"rom\": {\n");
    fprintf(fp, "    \"title\": ");
    json_write(fp, rom->title);
    fprintf(fp, ",\n");
    fprintf(fp, "    \"game_code\": ");
    json_write(fp, rom->game_code);
    fprintf(fp, ",\n");
    fprintf(fp, "    \"size\": %llu,\n", (unsigned long long)rom->size);
    fprintf(fp, "    \"sha1\": ");
    json_write(fp, sha1);
    fprintf(fp, "\n");
    fprintf(fp, "  },\n");
    fprintf(fp, "  \"nitrofs_files\": %d,\n", file_count);
    fprintf(fp, "  \"target\": {\n");
    fprintf(fp, "    \"path\": ");
    json_write(fp, PIT_TARGET_PATH);
    fprintf(fp, ",\n");
    fprintf(fp, "    \"offset\": %u\n", target_offset);
    fprintf(fp, "  },\n");
    fprintf(fp, "  \"sound_data_sdat\": %s,\n", sdat_found ? "true" : "false");
    fprintf(fp, "  \"exports\": [\"banner.png\", \"nitrofs.txt\", "
                "\"sound_data.sdat.txt\"]\n");
    fprintf(fp, "}\n");
    if (fclose(fp) != 0) {
        return -1;
    }
    return 0;
}

/* -------------------------------------------------------------------- driver */

int pit_ingest_run(const char *rom_path, const char *export_dir,
                   const pit_patch_info *info, pit_patch_log_fn log,
                   void *ctx)
{
    pit_rom rom;
    char sha1[PIT_SHA1_HEX_LEN];
    sdat_probe sdat;
    int file_count = 0;
    int failed = 0;

    if (rom_path == NULL || export_dir == NULL) {
        return -1;
    }
    memset(&sdat, 0, sizeof(sdat));

    if (pit_rom_open(&rom, rom_path) != 0) {
        return -1;
    }
    if (make_dirs(export_dir) != 0) {
        pit_rom_close(&rom);
        return -1;
    }
    ing_log(log, ctx, "Exporting generated data...");

    if (info && info->input_sha1[0] != '\0') {
        memcpy(sha1, info->input_sha1, PIT_SHA1_HEX_LEN);
    } else {
        compute_sha1(&rom, sha1);
    }

    if (export_banner(&rom, export_dir) != 0) {
        failed = 1;
    } else {
        char text[96];
        const pit_fs_entry *target = pit_rom_find(&rom, PIT_TARGET_PATH);

        snprintf(text, sizeof(text), "banner.png: icon and title exported%s.",
                 target ? "" : " (no target file)");
        ing_log(log, ctx, text);
    }

    if (export_nitrofs(&rom, export_dir, &file_count) != 0) {
        failed = 1;
    } else {
        char text[96];

        snprintf(text, sizeof(text), "nitrofs.txt: %d archive files.", file_count);
        ing_log(log, ctx, text);
    }

    if (export_sdat(&rom, export_dir, &sdat) != 0) {
        failed = 1;
    } else {
        char text[96];

        if (!sdat.found) {
            snprintf(text, sizeof(text), "sound_data.sdat: not found in this ROM.");
        } else if (!sdat.good) {
            snprintf(text, sizeof(text), "sound_data.sdat: present, header unreadable.");
        } else {
            snprintf(text, sizeof(text), "sound_data.sdat: SDAT v%u.%u, %u sections.",
                     sdat.version_major, sdat.version_minor, sdat.sections);
        }
        ing_log(log, ctx, text);
    }

    if (export_manifest(&rom, export_dir, info, sha1, file_count,
                        sdat.found) != 0) {
        failed = 1;
    }

    pit_rom_close(&rom);
    return failed ? -1 : 0;
}