/*
 * Headless driver for the patch pipeline.
 *
 * The SDL front end and the Android app call exactly the same
 * pit_patcher_run, so this binary exists to exercise the pipeline without a
 * window: it is what the packaging script and the round-trip test use to prove
 * that the plan still produces the expected bytes.
 */

#include <stdio.h>
#include <string.h>

#include "core/pit_ingest.h"
#include "core/pit_mods.h"
#include "core/pit_patcher.h"

typedef struct {
    int last_step;
} cli_state;

static void on_log(void *ctx, int step, const char *message, double fraction)
{
    cli_state *state = (cli_state *)ctx;

    if (step != state->last_step) {
        printf("[%d/%d] %s\n", step, PIT_PATCH_STEPS, message);
        state->last_step = step;
    } else {
        printf("      %s\n", message);
    }
    (void)fraction;
}

static void usage(const char *argv0)
{
    fprintf(stderr,
            "usage: %s [--no-mods] [--mods-dir <dir>] [--mod <id> | --list-mods]\n"
            "                [--ingest <dir>] <input.nds> <output.nds>\n"
            "\n"
            "Verifies a Partners in Time (EUR) ROM and writes a copy for the\n"
            "PiT decompilation project. Your OWN ROM is the only source of\n"
            "the game's assets and audio: nothing is downloaded or shipped.\n"
            "By default the %s data mod is applied to the copy; --no-mods\n"
            "verifies and copies without a mod. With --ingest, generated data\n"
            "(banner, NitroFS tree, SDAT probe, manifest) is exported to\n"
            "<dir>.\n"
            "\n"
            "Mods are discovered from <mods-dir>/<id>/profile.json, default\n"
            "\"mods\". --list-mods prints what was found; --mod <id> applies\n"
            "that profile instead of the built-in one.\n",
            argv0, "hard_mode");
}

static int list_mods(const char *root)
{
    pit_mod_catalog catalog;
    char error[256];
    unsigned int skipped = 0;
    unsigned int i;
    unsigned int j;
    int found;

    found = pit_mods_scan(root, &catalog, error, sizeof(error), &skipped);
    if (found < 0) {
        fprintf(stderr, "cannot read mods directory '%s': %s\n", root, error);
        return 1;
    }
    if (found == 0) {
        printf("no usable mod profiles under %s\n", root);
    }
    for (i = 0; i < catalog.count; i++) {
        const pit_mod_profile *profile = &catalog.profiles[i];

        printf("%s  (%s%s%s, %u transforms)\n", profile->name, profile->id,
               profile->version[0] ? ", v" : "", profile->version,
               profile->transform_count);
        for (j = 0; j < profile->transform_count; j++) {
            const pit_mod_transform *t = &profile->transforms[j];

            if (t->den == 1u) {
                printf("    %-12s x%u [%u..%u]\n", t->field, t->num,
                       t->min_value, t->max_value);
            } else {
                printf("    %-12s x%u/%u [%u..%u]\n", t->field, t->num, t->den,
                       t->min_value, t->max_value);
            }
        }
    }
    if (skipped > 0) {
        printf("%u profile(s) skipped; last reason: %s\n", skipped, error);
    }
    return 0;
}

int main(int argc, char **argv)
{
    cli_state state;
    pit_patch_info info;
    pit_patch_result result;
    int apply_plan = 1;
    int list_requested = 0;
    const char *ingest_dir = NULL;
    const char *mods_dir = "mods";
    const char *mod_id = NULL;
    pit_mod_profile profile;
    const pit_mod_profile *selected = NULL;
    const char *input;
    const char *output;

    while (argc > 1 && argv[1][0] == '-' && argv[1][0] != '\0') {
        if (strcmp(argv[1], "--no-mods") == 0) {
            apply_plan = 0;
            argc--;
            argv++;
        } else if (strcmp(argv[1], "--mods-dir") == 0 && argc > 2) {
            mods_dir = argv[2];
            argc -= 2;
            argv += 2;
        } else if (strcmp(argv[1], "--mod") == 0 && argc > 2) {
            mod_id = argv[2];
            argc -= 2;
            argv += 2;
        } else if (strcmp(argv[1], "--list-mods") == 0) {
            /*
             * Recorded rather than acted on, so "--mods-dir" is still honoured
             * when it follows this flag.
             */
            list_requested = 1;
            argc--;
            argv++;
        } else if (strcmp(argv[1], "--ingest") == 0 && argc > 2) {
            ingest_dir = argv[2];
            argc -= 2;
            argv += 2;
        } else {
            usage(argv[0]);
            return 2;
        }
    }
    if (list_requested) {
        return list_mods(mods_dir);
    }
    if (argc != 3) {
        usage(argv[0]);
        return 2;
    }
    input = argv[1];
    output = argv[2];

    if (mod_id) {
        pit_mod_catalog catalog;
        char error[256];
        unsigned int skipped = 0;
        unsigned int i;
        const pit_mod_profile *found;

        if (pit_mods_scan(mods_dir, &catalog, error, sizeof(error), &skipped) < 0) {
            fprintf(stderr, "cannot read mods directory '%s': %s\n", mods_dir,
                    error);
            return 2;
        }
        found = pit_mods_find(&catalog, mod_id);
        if (!found) {
            fprintf(stderr, "no usable mod with id '%s' under %s\n", mod_id,
                    mods_dir);
            if (catalog.count > 0) {
                fprintf(stderr, "available:");
                for (i = 0; i < catalog.count; i++) {
                    fprintf(stderr, " %s", catalog.profiles[i].id);
                }
                fprintf(stderr, "\n");
            }
            return 2;
        }
        profile = *found;
        selected = &profile;
        printf("mod profile: %s (%s), %u transforms\n", profile.name, profile.id,
               profile.transform_count);
    }

    memset(&state, 0, sizeof(state));
    memset(&info, 0, sizeof(info));

    result = pit_patcher_run_ex(input, output, apply_plan, selected, on_log,
                                &state, &info);
    printf("\n%s\n", pit_patch_result_text(result));
    if (result == PIT_PATCH_OK) {
        printf("plan applied    : %s\n", apply_plan ? "yes" : "no");
        printf("mod profile     : %s\n",
               selected ? selected->name : "built-in hard_mode");
        printf("records patched : %u\n", info.records_patched);
        printf("fields written  : %u\n", info.fields_written);
        printf("target offset   : 0x%08X\n", info.target_offset);
        printf("header CRC-16   : %04X\n", info.header_crc);
        printf("source SHA-1    : %s\n", info.input_sha1);
        if (ingest_dir) {
            printf("\n");
            if (pit_ingest_run(input, ingest_dir, &info, on_log, &state) != 0) {
                fprintf(stderr, "generated data export failed\n");
                return 1;
            }
            printf("generated data exported to %s\n", ingest_dir);
        }
    }
    return result == PIT_PATCH_OK ? 0 : 1;
}
