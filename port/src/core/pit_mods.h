/*
 * Mod profile discovery for the PiT launcher.
 *
 * A profile is a mods/<id>/profile.json file that describes how to scale fields
 * in the user's own stat table. It carries multipliers and nothing else: no ROM
 * bytes, no names, no stats. The launcher combines a profile with the data it
 * reads out of the user's cartridge.
 *
 * The split is deliberate. Record layout - which byte offset holds max_hp, how
 * long a record is, how many records exist - is a property of the retail
 * cartridge and the decompiled BattleEnemyStatRecord, so it stays compiled into
 * the launcher. A profile only names a field and a scale, which means a new mod
 * is a new folder and never a rebuild.
 *
 * Scales are kept as exact integer rationals. The generator that produces the
 * built-in Hard Mode table, the Python data-profile pipeline and this reader all
 * reduce a decimal literal to num/den the same way, so a value landing exactly
 * on a .5 boundary rounds identically in every implementation.
 */
#ifndef PIT_MODS_H
#define PIT_MODS_H

#include <stddef.h>

/*
 * Limits are maximum character counts, not buffer sizes, so they read the same
 * as the Java reader's ID_MAX and friends and the two agree at the boundary.
 * Every buffer below is one byte larger than its limit to leave room for the
 * terminator.
 */
#define PIT_MOD_ID_MAX         32
#define PIT_MOD_NAME_MAX       48
#define PIT_MOD_VERSION_MAX    16
#define PIT_MOD_DESC_MAX      200
#define PIT_MOD_DIR_MAX        64
#define PIT_MOD_FIELD_MAX      24
#define PIT_MOD_APPLIES_MAX    16
#define PIT_MOD_TRANSFORM_MAX  16
#define PIT_MOD_MAX            24

/* Schema identifier this reader understands. */
#define PIT_MOD_SCHEMA "pit-mod-profile-v1"

/*
 * Bounds on an accepted scale. They keep the 64-bit multiply in
 * pit_mods_scale() far away from overflow even for a hostile profile, and they
 * are far looser than any balance value a person would write by hand.
 */
#define PIT_MOD_NUM_MAX 1000000000u
#define PIT_MOD_DEN_MAX 1000000000u

typedef struct {
    /*
     * Field name, resolved against the launcher's own field table. A profile
     * cannot introduce an offset of its own, so it cannot write outside the
     * record it is scaling.
     */
    char field[PIT_MOD_FIELD_MAX + 1];
    unsigned int num;  /* scale is exactly num / den */
    unsigned int den;
    unsigned int min_value;
    unsigned int max_value;
    /*
     * Retained rather than dropped, so a disabled transform still shows in the
     * MODS list greyed out. Dropping it would silently hide a tuned field the
     * author turned off.
     */
    int enabled;
} pit_mod_transform;

typedef struct {
char id[PIT_MOD_ID_MAX + 1];
char name[PIT_MOD_NAME_MAX + 1];
char version[PIT_MOD_VERSION_MAX + 1];
char description[PIT_MOD_DESC_MAX + 1];
/* Owning folder, kept for diagnostics and as a stable sort tiebreak. */
char dir[PIT_MOD_DIR_MAX + 1];
char applies_to[PIT_MOD_APPLIES_MAX + 1];
    pit_mod_transform transforms[PIT_MOD_TRANSFORM_MAX];
    unsigned int transform_count;
} pit_mod_profile;

typedef struct {
    pit_mod_profile profiles[PIT_MOD_MAX];
    unsigned int count;
} pit_mod_catalog;

/*
 * Exact half-up scaling with a clamp, shared by the built-in plan and by every
 * discovered profile so both take the same rounding path.
 */
unsigned int pit_mods_scale(unsigned int value, unsigned int num,
                            unsigned int den, unsigned int min_value,
                            unsigned int max_value);

/*
 * Parse one profile file. Returns 0 on success and fills error with a short
 * reason on failure. A malformed or unsupported profile is reported, never
 * guessed at: silently dropping a transform would produce a ROM that looks
 * patched but is missing part of the mod.
 */
int pit_mods_load(const char *path, pit_mod_profile *out, char *error,
                  size_t error_size);

/*
 * Scan every profile.json one folder below root and collect the valid ones,
 * ordered by id so the selection index means the same thing on every platform.
 * Folders whose profile does not parse are skipped rather than fatal, so one
 * broken mod cannot hide the others; skipped receives that count when not NULL.
 * Returns the number of profiles found, or -1 if the root is unusable.
 */
int pit_mods_scan(const char *root, pit_mod_catalog *out, char *error,
                  size_t error_size, unsigned int *skipped);

/*
 * Finds a profile by its declared id, which is what the catalog lists and what
 * a mod author writes in profile.json. It is deliberately not the folder name:
 * those may differ, and the id is the stable identity. Returns NULL when no
 * profile matches.
 */
const pit_mod_profile *pit_mods_find(const pit_mod_catalog *catalog,
                                     const char *id);

/* Append <root>/<name>/profile.json to path. Returns 0 on success. */
int pit_mods_profile_path(char *path, size_t path_size, const char *root,
                          const char *name);

/* True when root contains at least one readable profile folder. */
int pit_mods_root_exists(const char *root);

#endif /* PIT_MODS_H */