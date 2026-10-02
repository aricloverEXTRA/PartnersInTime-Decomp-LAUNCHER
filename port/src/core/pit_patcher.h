/*
 * Verifies a Partners in Time ROM, applies a mod plan to the user's own copy
 * and writes a patched ROM.
 *
 * The patcher never ships or derives game data. It locates one file inside the
 * user's NitroFS, edits unsigned 16-bit stat fields in place using the record
 * layout recovered from the decompiled BattleEnemyStatRecord, and repairs the
 * DS header CRC so the result is a structurally valid ROM.
 *
 * Progress is reported through a callback so the SDL front end and the command
 * line can show the same steps. The Android build implements the identical
 * sequence against the generated plan data.
 */

#ifndef PIT_CORE_PIT_PATCHER_H
#define PIT_CORE_PIT_PATCHER_H

#include <stddef.h>

#include "pit_mods.h"

typedef enum {
    PIT_PATCH_OK = 0,
    PIT_PATCH_ERR_ARGS,
    PIT_PATCH_ERR_READ,          /* could not read the input ROM */
    PIT_PATCH_ERR_SIZE,          /* not a 64 MiB ROM */
    PIT_PATCH_ERR_SHA1,          /* hash does not match the supported ROM */
    PIT_PATCH_ERR_HEADER,        /* title or game code mismatch */
    PIT_PATCH_ERR_CRC,           /* stored header CRC does not verify */
    PIT_PATCH_ERR_TARGET,        /* NitroFS target not found */
    PIT_PATCH_ERR_TARGET_SIZE,   /* target is not record_count * record_size */
    PIT_PATCH_ERR_WRITE,         /* could not write the output ROM */
    PIT_PATCH_ERR_PLAN           /* no enabled transform in the plan */
} pit_patch_result;

/* Number of steps pit_patcher_run reports, for a progress bar. */
#define PIT_PATCH_STEPS 9

typedef struct {
    unsigned int target_offset;   /* absolute file offset of the target */
    unsigned int target_size;
    unsigned int records_patched;
    unsigned int fields_written;
    unsigned int header_crc;      /* CRC written into the patched header */
    char         input_sha1[41];
} pit_patch_info;

/*
 * Reports one step. step is 1..PIT_PATCH_STEPS, message is a short line for the
 * log pane, and fraction is overall progress in 0..1. Safe to call with log
 * set to NULL.
 */
typedef void (*pit_patch_log_fn)(void *ctx, int step, const char *message,
                                 double fraction);

const char *pit_patch_result_text(pit_patch_result result);

/*
 * Runs the full pipeline. Verifies the input, copies it, applies the plan to the
 * copy (unless apply_plan is zero), recomputes the header CRC and writes
 * output_path. On failure nothing is written and the reason is reported through
 * the log.
 *
 * With apply_plan == 0 the copy is written untouched apart from the repaired
 * header CRC, so the tool doubles as a plain "prepare my own ROM" step and a
 * data mod, and the mods stay optional.
 */
pit_patch_result pit_patcher_run(const char *input_path, const char *output_path,
                                 int apply_plan, pit_patch_log_fn log,
                                 void *ctx, pit_patch_info *info);

/*
 * The same pipeline with an explicit mod profile. A NULL profile means the
 * built-in plan, which is what pit_patcher_run passes, so the shipped default
 * path is unchanged and still produces the same bytes.
 *
 * Only the transform set differs between the two. Record layout, the field
 * offsets and the scaling rule all stay owned by the launcher, so a profile can
 * choose multipliers but cannot write outside the stat record.
 */
pit_patch_result pit_patcher_run_ex(const char *input_path,
                                    const char *output_path, int apply_plan,
                                    const pit_mod_profile *profile,
                                    pit_patch_log_fn log, void *ctx,
                                    pit_patch_info *info);

/*
 * CRC-16/MODBUS over the first 0x15E header bytes, as this release stores it at
 * 0x15E. The seed is 0xFFFF rather than the 0 the DS documentation describes;
 * see pit_patcher.c for the check against the cartridge.
 */
unsigned int pit_patch_header_crc16(const unsigned char *data, size_t size);

#endif /* PIT_CORE_PIT_PATCHER_H */
