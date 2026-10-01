/*
 * Generated-data export for the PiT decompilation.
 *
 * Exports, from the user's own cartridge, the data the reconstruction reads:
 * the banner as a PNG, the NitroFS file tree, a probe of the sound bank, and a
 * manifest describing all of it. Every byte written comes from the user's own
 * ROM; the ROM itself is never modified and nothing is shipped or committed.
 */

#ifndef PIT_CORE_PIT_INGEST_H
#define PIT_CORE_PIT_INGEST_H

#include "pit_patcher.h"

/*
 * Runs the export. rom_path is the source cartridge and export_dir the directory
 * the four exports are written into (created if missing). info may carry fields
 * computed by an earlier patcher step; when it is NULL or its input_sha1 is
 * empty, the SHA-1 is recomputed from the ROM.
 *
 * Progress is reported through log at step PIT_PATCH_STEPS with fraction 0.98;
 * no line is emitted for the final result, so the caller states success or
 * failure. Returns 0 when all four exports were written, -1 otherwise.
 */
int pit_ingest_run(const char *rom_path, const char *export_dir,
                   const pit_patch_info *info, pit_patch_log_fn log,
                   void *ctx);

#endif /* PIT_CORE_PIT_INGEST_H */