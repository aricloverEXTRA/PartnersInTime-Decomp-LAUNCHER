#include "core/pit.h"
#include "core/pit_archive.h"
#include "core/pit_gfx.h"
#include "core/pit_nds_banner.h"
#include "core/pit_png.h"
#include "core/pit_rom.h"
#include "core/pit_sha1.h"
#include "core/pit_video.h"
#include "platform/pit_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct {
    const char *sha1;
    const char *region;
} k_known_roms[] = {
    { "ba4ec2f99b4f2e0047601552bccf00aa73e28701", "EUR" },
    { "89c9136db3c3975c451a907e8bd6861ce6b81557", "USA" },
    { "e23db7ff44d38299a6f1c377780378b191592a7e", "USA (Rev 1)" }
};

#define PIT_KNOWN_ROM_COUNT (sizeof(k_known_roms) / sizeof(k_known_roms[0]))

static void print_rom_notice(void)
{
    printf(
        "Partners in Time Port\n"
        "======================\n"
        "This program contains no Nintendo assets.\n"
        "\n"
        "You must supply your own legally obtained copy of\n"
        "Mario & Luigi: Partners in Time (Nintendo DS).\n"
        "\n"
        "Graphics, audio, text, and other game data are the property\n"
        "of Nintendo. They are read at runtime from your own ROM and\n"
        "are never written into this program.\n"
        "\n");
}

static void print_usage(void)
{
    printf("usage: pit [--no-assets] [--dump-nitrofs] [--dump-archive <path>]\n");
    printf("            [--render-banner <out.png>] [rom-path]\n");
    printf("  --no-assets        run the presentation shell with placeholder content\n");
    printf("  --dump-nitrofs     list the ROM's NitroFS archive and exit\n");
    printf("  --dump-archive     list the entries of one .dat container and exit\n");
    printf("  --render-banner    decode the cartridge banner and write it to a PNG\n");
    printf("  rom-path           path to your ROM (or set PIT_ROM)\n");
}

/*
 * Decodes the cartridge banner and writes the 32x32 icon out as a PNG.
 *
 * This is the renderer's first end-to-end check against real data: it exercises
 * the ROM reader, the BGR555 palette conversion, the 4bpp tile decoder and the
 * PNG writer, none of which depend on the game's own resource containers.
 */
static int render_banner(const char *rom_path, const char *out_path)
{
    pit_rom rom;
    pit_banner banner;
    int i;

    if (pit_rom_open(&rom, rom_path) != 0) {
        printf("error: cannot parse ROM: %s\n", rom_path);
        return 1;
    }
    if (pit_banner_decode(&banner, &rom) != 0) {
        printf("error: no usable banner in this ROM\n");
        pit_rom_close(&rom);
        return 1;
    }

    printf("banner    : 0x%08x +0x%x\n", banner.offset, banner.size);
    printf("version   : 0x%04x\n", banner.version);
    if (banner.crc_ok) {
        printf("crc16     : stored 0x%04x computed 0x%04x (ok)\n",
               banner.crc16_stored, banner.crc16_computed);
    } else {
        printf("crc16     : stored 0x%04x, not reproducible by CCITT-FALSE\n",
               banner.crc16_stored);
        printf("            (vendor-specific value; not a data integrity check)\n");
    }

    printf("palette   :");
    for (i = 0; i < PIT_BANNER_COLORS; i++) {
        printf(" %06X", banner.palette[i] & 0xFFFFFFu);
    }
    printf("\n");

    printf("titles    :\n");
    printf("  japanese: %s\n", banner.japanese_title);
    printf("  english : %s\n", banner.english_title);
    printf("  french  : %s\n", banner.french_title);
    printf("  german  : %s\n", banner.german_title);
    printf("  italian : %s\n", banner.italian_title);
    printf("  spanish : %s\n", banner.spanish_title);

    if (pit_png_write(out_path, &banner.icon) != 0) {
        printf("error: cannot write PNG: %s\n", out_path);
        pit_banner_free(&banner);
        pit_rom_close(&rom);
        return 1;
    }
    printf("wrote     : %s (%dx%d)\n", out_path, banner.icon.width, banner.icon.height);

    {
        pit_screen *screen = (pit_screen *)calloc(1, sizeof(pit_screen));
        if (screen != NULL) {
            int y;
            for (y = 0; y < PIT_SCREEN_HEIGHT; y++) {
                int x;
                for (x = 0; x < PIT_SCREEN_WIDTH; x++) {
                    screen->data[y][x] = 0xFF101018u;
                }
            }
            if (pit_banner_draw(&banner, screen) == 0) {
                printf("blit      : icon centred on a %dx%d screen, ok\n",
                       PIT_SCREEN_WIDTH, PIT_SCREEN_HEIGHT);
            } else {
                printf("blit      : failed\n");
            }
            free(screen);
        }
    }

    pit_banner_free(&banner);
    pit_rom_close(&rom);
    return 0;
}

/*
 * Walks one `.dat` container out of the archive and reports its entry table.
 * This is the outer offset-only container shared by the game's data files.
 */
static int dump_archive(const char *rom_path, const char *member)
{
    pit_rom rom;
    pit_archive archive;
    unsigned char *bytes;
    const unsigned char *view;
    size_t size = 0;
    unsigned int i;

    if (pit_rom_open(&rom, rom_path) != 0) {
        printf("error: cannot parse ROM: %s\n", rom_path);
        return 1;
    }
    bytes = (unsigned char *)pit_rom_read(&rom, member, &size);
    if (bytes == NULL) {
        printf("error: not in archive: %s\n", member);
        pit_rom_close(&rom);
        return 1;
    }

    printf("%s: %llu bytes\n", member, (unsigned long long)size);
    if (pit_archive_open(&archive, bytes, size) != 0) {
        view = bytes;
        printf("  not an offset archive (first word 0x%08x)\n",
               size >= 4 ? (unsigned int)view[0] | ((unsigned int)view[1] << 8) |
                                ((unsigned int)view[2] << 16) | ((unsigned int)view[3] << 24)
                          : 0u);
        free(bytes);
        pit_rom_close(&rom);
        return 1;
    }

    printf("  %u entries, table 0x%x\n", archive.count, archive.count * 4u + 4u);
    for (i = 0; i < archive.count; i++) {
        const unsigned char *entry;
        size_t entry_size = 0;
        int b;

        if (pit_archive_entry(&archive, i, &entry, &entry_size) != 0) {
            break;
        }
        printf("%8llu  [%3u] ", (unsigned long long)entry_size, i);
        for (b = 0; b < 16 && (size_t)b < entry_size; b++) {
            printf("%02X", entry[b]);
        }
        printf("\n");
    }

    free(bytes);
    pit_rom_close(&rom);
    return 0;
}

/* Walks the archive and prints it, so ingest can be checked without a display. */
static int dump_nitrofs(const char *path)
{
    pit_rom rom;
    pit_fs_entry *entries;
    int n, i;
    unsigned long long total = 0;

    if (pit_rom_open(&rom, path) != 0) {
        printf("error: cannot parse ROM: %s\n", path);
        return 1;
    }

    printf("title      : %s\n", rom.title);
    printf("game code  : %s  (maker %s)\n", rom.game_code, rom.maker_code);
    printf("arm9       : 0x%08x +0x%x\n", rom.arm9_offset, rom.arm9_size);
    printf("arm7       : 0x%08x +0x%x\n", rom.arm7_offset, rom.arm7_size);
    printf("fnt        : 0x%08x +0x%x\n", rom.fnt_offset, rom.fnt_size);
    printf("fat        : 0x%08x +0x%x (%u entries)\n", rom.fat_offset,
           rom.fat_size, rom.file_count);
    printf("banner     : 0x%08x +0x%x\n", rom.banner_offset, rom.banner_size);
    printf("first file : %u\n\n", rom.first_file_id);

    entries = (pit_fs_entry *)calloc(2048, sizeof(pit_fs_entry));
    if (entries == NULL) {
        pit_rom_close(&rom);
        printf("error: out of memory\n");
        return 1;
    }

    n = pit_rom_list(&rom, 0, entries, 2048);
    if (n < 0) {
        printf("error: NitroFS walk failed\n");
        free(entries);
        pit_rom_close(&rom);
        return 1;
    }

    for (i = 0; i < n; i++) {
        printf("%10u  %s\n", entries[i].size, entries[i].path);
        total += entries[i].size;
    }
    printf("\n%d files, %llu bytes total\n", n, total);

    free(entries);
    pit_rom_close(&rom);
    return 0;
}

static int verify_rom(const char *path)
{
    char hex[PIT_SHA1_HEX_LEN];
    size_t i;
    int matched = 0;

    if (pit_sha1_file_hex(path, hex) != 0) {
        printf("error: cannot read ROM: %s\n", path);
        return -1;
    }

    printf("rom sha1: %s\n", hex);

    for (i = 0; i < PIT_KNOWN_ROM_COUNT; i++) {
        if (strcmp(hex, k_known_roms[i].sha1) == 0) {
            printf("rom match: %s (supported)\n", k_known_roms[i].region);
            matched = 1;
            break;
        }
    }

    if (!matched) {
        printf("error: unrecognised ROM.\n");
        printf("       This port only supports known, unmodified ROM revisions.\n");
        return -1;
    }

    return 0;
}

static void fill_placeholder(pit_frame *frame, unsigned int tick)
{
    int s, x, y;

    for (s = 0; s < PIT_SCREEN_COUNT; s++) {
        pit_screen *screen = &frame->screen[s];
        int bar = (int)((tick * 2u + (unsigned)s * 120u) % (unsigned)(PIT_SCREEN_WIDTH + 64u)) - 32;

        for (y = 0; y < PIT_SCREEN_HEIGHT; y++) {
            for (x = 0; x < PIT_SCREEN_WIDTH; x++) {
                int checker = ((x / 8) + (y / 8)) & 1;
                int in_bar = (x >= bar && x < bar + 64);
                unsigned int color;

                if (s == PIT_SCREEN_ENGINE) {
                    color = checker ? 0xFF16324Fu : 0xFF1E88E5u;
                } else {
                    color = checker ? 0xFF1B4332u : 0xFF43A047u;
                }

                if (in_bar) {
                    color = 0xFFFFD040u;
                }

                screen->data[y][x] = color;
            }
        }
    }
}

int main(int argc, char **argv)
{
    pit_frame *frame;
    pit_present_info info;
    const char *rom_path = NULL;
    const char *archive_member = NULL;
    const char *banner_out = NULL;
    int no_assets = 0;
    int dump_fs = 0;
    int i;
    unsigned int tick = 0;

    setvbuf(stdout, NULL, _IONBF, 0);

    print_rom_notice();

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--no-assets") == 0) {
            no_assets = 1;
        } else if (strcmp(argv[i], "--dump-nitrofs") == 0) {
            dump_fs = 1;
        } else if (strcmp(argv[i], "--dump-archive") == 0 && i + 1 < argc) {
            archive_member = argv[++i];
        } else if (strcmp(argv[i], "--render-banner") == 0 && i + 1 < argc) {
            banner_out = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage();
            return 0;
        } else {
            rom_path = argv[i];
        }
    }

    if (rom_path == NULL) {
        rom_path = getenv("PIT_ROM");
    }

    if (rom_path == NULL && !no_assets) {
        printf("error: no ROM supplied.\n\n");
        print_usage();
        return 2;
    }

    if (dump_fs) {
        return dump_nitrofs(rom_path);
    }

    if (banner_out != NULL) {
        if (rom_path == NULL) {
            printf("error: no ROM supplied.\n\n");
            print_usage();
            return 2;
        }
        return render_banner(rom_path, banner_out);
    }

    if (archive_member != NULL) {
        if (rom_path == NULL) {
            printf("error: no ROM supplied.\n\n");
            print_usage();
            return 2;
        }
        return dump_archive(rom_path, archive_member);
    }

    if (!no_assets) {
        if (verify_rom(rom_path) != 0) {
            return 2;
        }
    } else {
        printf("running with placeholder content (no ROM, no game assets)\n\n");
    }

    frame = (pit_frame *)calloc(1, sizeof(pit_frame));
    if (frame == NULL) {
        printf("error: out of memory\n");
        return 1;
    }

    pit_video_init();

    if (pit_platform_video_init("Partners in Time Port") != 0) {
        printf("error: cannot initialise video\n");
        free(frame);
        return 1;
    }

    printf("F1 layout (stacked/side-by-side/overlay)  F2 dynamic screen  Esc quit\n");

    while (!pit_platform_quit_requested()) {
        pit_platform_poll();
        if (pit_platform_quit_requested()) {
            break;
        }

        fill_placeholder(frame, tick);

        if (pit_video_focus_mode() == PIT_FOCUS_AUTO) {
            pit_video_set_active_screen(
                ((tick / 120u) % 2u == 0u) ? PIT_SCREEN_ENGINE : PIT_SCREEN_ACTION);
        }

        pit_video_step();
        pit_video_fill_info(&info);
        pit_platform_present(frame, &info);

        tick++;
        pit_platform_delay_ms(16);
    }

    pit_platform_video_shutdown();
    free(frame);
    return 0;
}
