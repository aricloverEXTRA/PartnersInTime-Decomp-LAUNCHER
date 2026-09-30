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
            "usage: %s [--no-mods] <input.nds> <output.nds>\n"
            "\n"
            "Verifies a Partners in Time (EUR) ROM and writes a prepared copy\n"
            "for the PiT decompilation project. By default the %s data mod is\n"
            "applied to the user's own copy; --no-mods verifies and copies the\n"
            "ROM without applying any mod.\n",
            argv0, "hard_mode");
}

int main(int argc, char **argv)
{
    cli_state state;
    pit_patch_info info;
    pit_patch_result result;
    int apply_plan = 1;
    const char *input;
    const char *output;

    while (argc > 1 && argv[1][0] == '-' && argv[1][0] != '\0') {
        if (strcmp(argv[1], "--no-mods") == 0) {
            apply_plan = 0;
        } else {
            usage(argv[0]);
            return 2;
        }
        argc--;
        argv++;
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
    }
    return result == PIT_PATCH_OK ? 0 : 1;
}
