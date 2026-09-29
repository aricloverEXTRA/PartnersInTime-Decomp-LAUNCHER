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
            "usage: %s <input.nds> <output.nds>\n"
            "\n"
            "Verifies a Partners in Time (EUR) ROM, applies the %s plan to\n"
            "the user's own copy and writes a patched ROM.\n",
            argv0, "hard_mode");
}

int main(int argc, char **argv)
{
    cli_state state;
    pit_patch_info info;
    pit_patch_result result;

    if (argc != 3) {
        usage(argv[0]);
        return 2;
    }

    memset(&state, 0, sizeof(state));
    memset(&info, 0, sizeof(info));

    result = pit_patcher_run(argv[1], argv[2], on_log, &state, &info);
    printf("\n%s\n", pit_patch_result_text(result));
    if (result == PIT_PATCH_OK) {
        printf("records patched : %u\n", info.records_patched);
        printf("fields written  : %u\n", info.fields_written);
        printf("target offset   : 0x%08X\n", info.target_offset);
        printf("header CRC-16   : %04X\n", info.header_crc);
        printf("source SHA-1    : %s\n", info.input_sha1);
    }
    return result == PIT_PATCH_OK ? 0 : 1;
}
