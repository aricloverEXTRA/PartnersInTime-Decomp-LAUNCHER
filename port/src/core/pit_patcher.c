/*
 * Patcher pipeline shared by the Windows front end and the command line.
 *
 * The pipeline is deliberately linear and each step is reported before and
 * after it runs, because the whole point of the UI is that the user can see
 * what happened to their ROM rather than trusting a silent rewrite.
 */

#include "pit_patcher.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pit_patch_data.h"
#include "pit_rom.h"
#include "pit_sha1.h"

#define HEADER_CRC_OFFSET 0x15E
#define HEADER_CRC_SPAN   0x15E

/* ------------------------------------------------------------------ helpers */

static void log_step(pit_patch_log_fn log, void *ctx, int step,
                     const char *message, double fraction)
{
    if (log) {
        log(ctx, step, message, fraction);
    }
}

static unsigned int read_u16(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static void write_u16(unsigned char *p, unsigned int value)
{
    p[0] = (unsigned char)(value & 0xFFu);
    p[1] = (unsigned char)((value >> 8) & 0xFFu);
}

unsigned int pit_patch_header_crc16(const unsigned char *data, size_t size)
{
    unsigned int crc = 0xFFFFu;
    size_t i;

    /*
     * Init 0xFFFF, not 0. The DS documentation describes the header CRC with a
     * 0 initial value, but the EUR release stores the MODBUS variant: over
     * 0x000..0x15D the stored value is 0xD0BC, which only the 0xFFFF seed
     * reproduces. Using 0 yields 0xCB70 and would reject every real ROM, so the
     * seed is verified against the cartridge rather than assumed.
     */
    if (size > HEADER_CRC_SPAN) {
        size = HEADER_CRC_SPAN;
    }
    for (i = 0; i < size; i++) {
        unsigned int bit;

        crc ^= (unsigned int)data[i];
        for (bit = 0; bit < 8; bit++) {
            if (crc & 1u) {
                crc = (crc >> 1) ^ 0xA001u;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc & 0xFFFFu;
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

const char *pit_patch_result_text(pit_patch_result result)
{
    switch (result) {
    case PIT_PATCH_OK:              return "Patched ROM written.";
    case PIT_PATCH_ERR_ARGS:        return "No input ROM was selected.";
    case PIT_PATCH_ERR_READ:        return "The input ROM could not be read.";
    case PIT_PATCH_ERR_SIZE:        return "That file is not a 64 MiB NDS ROM.";
    case PIT_PATCH_ERR_SHA1:        return "Unsupported ROM: SHA-1 does not match the EUR release.";
    case PIT_PATCH_ERR_HEADER:      return "Unsupported ROM: cartridge title or game code mismatch.";
    case PIT_PATCH_ERR_CRC:         return "Cartridge header CRC-16 does not verify.";
    case PIT_PATCH_ERR_TARGET:      return "Target file is missing from this ROM's NitroFS.";
    case PIT_PATCH_ERR_TARGET_SIZE: return "Target file is not the expected record table.";
    case PIT_PATCH_ERR_WRITE:       return "The patched ROM could not be written.";
    case PIT_PATCH_ERR_PLAN:        return "The patch plan has no enabled transforms.";
    default:                       return "Unknown error.";
    }
}

/*
 * Applies one scale exactly.
 *
 * The plan stores scales as integer rationals, so this is half-up rounding on
 * integers with no floating point and no rounding-mode dependency. The Python
 * pipeline that produced the original profile export uses Decimal with
 * ROUND_HALF_UP, and all three agree because the rationals are the exact
 * decimal values (1.1 is 11/10, not the nearest double).
 */
static unsigned int scale_value(unsigned int value, unsigned int num,
                                unsigned int den, unsigned int min_value,
                                unsigned int max_value)
{
    unsigned long long scaled =
        ((unsigned long long)value * (unsigned long long)num +
         (unsigned long long)(den / 2u)) / (unsigned long long)den;

    if (scaled < (unsigned long long)min_value) {
        return min_value;
    }
    if (scaled > (unsigned long long)max_value) {
        return max_value;
    }
    return (unsigned int)scaled;
}

/* ----------------------------------------------------------------- pipeline */

pit_patch_result pit_patcher_run(const char *input_path, const char *output_path,
                                 pit_patch_log_fn log, void *ctx,
                                 pit_patch_info *info)
{
    unsigned char *patched = NULL;
    const unsigned char *data = NULL;
    size_t size = 0;
    pit_rom rom;
    const pit_fs_entry *target;
    pit_sha1_ctx sha;
    unsigned char digest[PIT_SHA1_DIGEST_LEN];
    char message[256];
    char actual[PIT_SHA1_HEX_LEN];
    unsigned int record;
    unsigned int i;
    unsigned int field;
    unsigned int written = 0;

    if (info) {
        memset(info, 0, sizeof(*info));
    }
    if (!input_path || !output_path) {
        return PIT_PATCH_ERR_ARGS;
    }

    /*
     * pit_rom_open loads the whole image and parses the NitroFS tables in one
     * pass, so the same buffer serves the hash, the header checks, the record
     * scan and the patched copy. Reading the 64 MiB file twice would double the
     * cost of the one operation the user actually waits for.
     */
    log_step(log, ctx, 1, "Reading source ROM...", 0.02);
    if (pit_rom_open(&rom, input_path) != 0) {
        log_step(log, ctx, 1, "Could not read the source ROM.", 0.02);
        return PIT_PATCH_ERR_READ;
    }
    data = rom.data;
    size = rom.size;

    /* Step 2: size. */
    log_step(log, ctx, 2, "Checking cartridge size...", 0.08);
    if (size != (size_t)PIT_PLAN_ROM_SIZE) {
        snprintf(message, sizeof(message),
                 "Expected %lld bytes, found %llu.",
                 (long long)PIT_PLAN_ROM_SIZE, (unsigned long long)size);
        log_step(log, ctx, 2, message, 0.08);
        pit_rom_close(&rom);
        return PIT_PATCH_ERR_SIZE;
    }

    /* Step 3: SHA-1 of the untouched source. */
    log_step(log, ctx, 3, "Computing SHA-1...", 0.16);
    pit_sha1_init(&sha);
    pit_sha1_update(&sha, data, size);
    pit_sha1_final(&sha, digest);
    /*
     * The digest goes in its own buffer: formatting the mismatch message into
     * the same buffer that holds the digest would pass an overlapping source
     * and destination to snprintf, which is undefined behaviour.
     */
    hex_digest(digest, actual);
    if (info) {
        memcpy(info->input_sha1, actual, sizeof(info->input_sha1));
    }
    if (strcmp(actual, PIT_PLAN_ROM_SHA1) != 0) {
        snprintf(message, sizeof(message), "Expected %s, got %s.",
                 PIT_PLAN_ROM_SHA1, actual);
        log_step(log, ctx, 3, message, 0.16);
        pit_rom_close(&rom);
        return PIT_PATCH_ERR_SHA1;
    }
    log_step(log, ctx, 3, "SHA-1 matches the supported EUR release.", 0.24);

    /* Step 4: cartridge identity. */
    log_step(log, ctx, 4, "Reading cartridge header...", 0.30);
    if (memcmp(data, PIT_PLAN_ROM_TITLE, strlen(PIT_PLAN_ROM_TITLE)) != 0 ||
        memcmp(data + 12, PIT_PLAN_ROM_GAME_CODE,
               strlen(PIT_PLAN_ROM_GAME_CODE)) != 0) {
        log_step(log, ctx, 4, "Cartridge title or game code mismatch.", 0.30);
        pit_rom_close(&rom);
        return PIT_PATCH_ERR_HEADER;
    }
    log_step(log, ctx, 4, "Title MARIO&LUIGI2 (ARMP) confirmed.", 0.36);

    /* Step 5: header CRC, so a damaged source is rejected before we write. */
    log_step(log, ctx, 5, "Verifying header CRC-16...", 0.42);
    {
        unsigned int stored = read_u16(data + HEADER_CRC_OFFSET);
        unsigned int computed = pit_patch_header_crc16(data, size);

        if (stored != computed) {
            snprintf(message, sizeof(message),
                     "Header CRC mismatch: stored %04X, computed %04X.",
                     stored, computed);
            log_step(log, ctx, 5, message, 0.42);
            pit_rom_close(&rom);
            return PIT_PATCH_ERR_CRC;
        }
        snprintf(message, sizeof(message), "Header CRC-16 %04X verified.", stored);
        log_step(log, ctx, 5, message, 0.46);
    }

    /* Step 6: locate the target inside the user's own NitroFS. */
    log_step(log, ctx, 6, "Scanning NitroFS for the stat table...", 0.50);
    target = pit_rom_find(&rom, PIT_TARGET_PATH);
    if (!target) {
        snprintf(message, sizeof(message), "%s is not present in this ROM.",
                 PIT_TARGET_PATH);
        log_step(log, ctx, 6, message, 0.50);
        pit_rom_close(&rom);
        return PIT_PATCH_ERR_TARGET;
    }
    if (target->size != PIT_RECORD_COUNT * PIT_RECORD_SIZE) {
        snprintf(message, sizeof(message),
                 "%s is %u bytes, expected %u.",
                 PIT_TARGET_PATH, target->size,
                 (unsigned int)(PIT_RECORD_COUNT * PIT_RECORD_SIZE));
        log_step(log, ctx, 6, message, 0.50);
        pit_rom_close(&rom);
        return PIT_PATCH_ERR_TARGET_SIZE;
    }
    if ((size_t)target->offset + target->size > size) {
        log_step(log, ctx, 6, "Target file extends past the end of the ROM.", 0.50);
        pit_rom_close(&rom);
        return PIT_PATCH_ERR_TARGET;
    }
    if (info) {
        info->target_offset = target->offset;
        info->target_size = target->size;
    }
    snprintf(message, sizeof(message),
             "Found %s at 0x%08X (%u records x %u bytes).",
             PIT_TARGET_PATH, target->offset,
             (unsigned int)PIT_RECORD_COUNT, (unsigned int)PIT_RECORD_SIZE);
    log_step(log, ctx, 6, message, 0.56);

    /* Step 7: patch a private copy. */
    log_step(log, ctx, 7, "Applying plan to a copy...", 0.62);
    patched = (unsigned char *)malloc(size);
    if (!patched) {
        pit_rom_close(&rom);
        return PIT_PATCH_ERR_WRITE;
    }
    memcpy(patched, data, size);
    pit_rom_close(&rom);
    data = NULL;

    for (record = 0; record < PIT_RECORD_COUNT; record++) {
        size_t base = (size_t)info->target_offset +
                      (size_t)record * (size_t)PIT_RECORD_SIZE;

        for (i = 0; i < PIT_TRANSFORM_COUNT; i++) {
            const pit_plan_transform *t = &PIT_PLAN_TRANSFORMS[i];
            unsigned int offset = PIT_RECORD_SIZE;
            unsigned int value;
            unsigned int result_value;

            for (field = 0; field < PIT_FIELD_COUNT; field++) {
                if (strcmp(PIT_PLAN_FIELDS[field].name, t->field) == 0) {
                    offset = PIT_PLAN_FIELDS[field].offset;
                    break;
                }
            }
            if (offset == PIT_RECORD_SIZE) {
                continue;
            }
            value = read_u16(patched + base + offset);
            result_value = scale_value(value, t->num, t->den,
                                       t->min_value, t->max_value);
            write_u16(patched + base + offset, result_value);
            written++;
        }

        if ((record % 8u) == 0u || record + 1u == PIT_RECORD_COUNT) {
            snprintf(message, sizeof(message),
                     "Patched record %u/%u...", record + 1u,
                     (unsigned int)PIT_RECORD_COUNT);
            log_step(log, ctx, 7, message,
                     0.62 + 0.24 * ((double)(record + 1u) /
                                    (double)PIT_RECORD_COUNT));
        }
    }
    log_step(log, ctx, 7, "All 98 records patched.", 0.86);
    if (info) {
        info->records_patched = PIT_RECORD_COUNT;
        info->fields_written = written;
    }

    /* Step 8: repair the header CRC over the patched bytes. */
    log_step(log, ctx, 8, "Recomputing header CRC-16...", 0.90);
    {
        unsigned int crc = pit_patch_header_crc16(patched, size);

        write_u16(patched + HEADER_CRC_OFFSET, crc);
        if (info) {
            info->header_crc = crc;
        }
        snprintf(message, sizeof(message), "Header CRC-16 set to %04X.", crc);
        log_step(log, ctx, 8, message, 0.92);
    }

    /* Step 9: write the result. */
    log_step(log, ctx, 9, "Writing patched ROM...", 0.94);
    {
        FILE *fp = fopen(output_path, "wb");
        size_t written_bytes = 0;

        if (!fp) {
            log_step(log, ctx, 9, "Could not open the output file.", 0.94);
            free(patched);
            return PIT_PATCH_ERR_WRITE;
        }
        while (written_bytes < size) {
            size_t chunk = size - written_bytes;
            size_t done;

            if (chunk > (size_t)(1u << 20)) {
                chunk = (size_t)(1u << 20);
            }
            done = fwrite(patched + written_bytes, 1, chunk, fp);
            if (done != chunk) {
                fclose(fp);
                remove(output_path);
                log_step(log, ctx, 9, "Write failed; the output was removed.", 0.94);
                free(patched);
                return PIT_PATCH_ERR_WRITE;
            }
            written_bytes += done;
            snprintf(message, sizeof(message), "Writing %llu%%...",
                     (unsigned long long)(100u * written_bytes / size));
            log_step(log, ctx, 9, message,
                     0.94 + 0.05 * ((double)written_bytes / (double)size));
        }
        if (fclose(fp) != 0) {
            remove(output_path);
            free(patched);
            return PIT_PATCH_ERR_WRITE;
        }
    }
    free(patched);
    log_step(log, ctx, 9, "Patched ROM written.", 1.0);
    return PIT_PATCH_OK;
}
