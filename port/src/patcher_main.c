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
            "usage: %s [--no-mods] [--ingest <dir>] <input.nds> <output.nds>\n"
            "\n"
            "Verifies a Partners in Time (EUR) ROM and writes a copy for the\n"
            "PiT decompilation project. Your OWN ROM is the only source of\n"
            "the game's assets and audio: nothing is downloaded or shipped.\n"
            "By default the %s data mod is applied to the copy; --no-mods\n"
            "verifies and copies without a mod. With --ingest, generated data\n"
            "(banner, NitroFS tree, SDAT probe, manifest) is exported to\n"
            "<dir>.\n",
            argv0, "hard_mode");
}

int main(int argc, char **argv)
{
    cli_state state;
    pit_patch_info info;
    pit_patch_result result;
    int apply_plan = 1;
    const char *ingest_dir = NULL;
    const char *input;
    const char *output;

    while (argc > 1 && argv[1][0] == '-' && argv[1][0] != '\0') {
        if (strcmp(argv[1], "--no-mods") == 0) {
            apply_plan = 0;
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
    if (argc != 3) {
        usage(argv[0]);
        return 2;
    }
    input = argv[1];
    output = argv[2];

    memset(&state, 0, sizeof(state));
    memset(&info, 0, sizeof(info));

    result = pit_patcher_run(input, output, apply_plan, on_log, &state, &info);
    printf("\n%s\n", pit_patch_result_text(result));
    if (result == PIT_PATCH_OK) {
        printf("plan applied    : %s\n", apply_plan ? "yes" : "no");
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
