/*
 * Mod profile discovery for the PiT launcher.
 *
 * This reader exists so that adding a mod is a matter of dropping a folder into
 * mods/, not of editing a generated header and rebuilding both the C and the
 * Java launcher. It understands only the profile schema, and it mirrors
 * port/tools/make_data_profile.py wherever the two could otherwise disagree:
 *
 *   - the schema must be "pit-mod-profile-v1";
 *   - "field" and a positive "scale" are required, and both are validated
 *     before "enabled" is consulted, so a disabled transform with a missing
 *     scale is still an error;
 *   - "enabled" defaults to true, "min" to 0 and "max" to 65535;
 *   - transforms keep their profile order, and a profile with no enabled
 *     transform is rejected.
 *
 * Two deliberate tightenings. A negative "min" is rejected rather than accepted,
 * and "min" is held to 65535 like "max", because this launcher writes 16-bit
 * fields and a larger bound would silently wrap. Neither affects a profile whose
 * bounds are already inside the field.
 */

#include "core/pit_mods.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32)
/*
 * FindFirstFileA is used rather than _findfirst because MinGW expands
 * _findfirst to _findfirst64 under _FILE_OFFSET_BITS, while _finddata_t stays
 * the narrower 32-bit struct. The pair therefore writes a 64-bit record into a
 * 32-bit one and returns a nonsense handle. WIN32_FIND_DATAA names the same
 * type on MSVC and MinGW, so there is no aliasing to get wrong.
 */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dirent.h>
#endif

#define MOD_U16_MAX     65535u
#define MOD_JSON_DEPTH  8
#define MOD_KEY_MAX     64
#define MOD_FILE_MAX    (1u << 20)
#define MOD_PATH_MAX    640

/* ------------------------------------------------------------- json reading */

static const char *skip_ws(const char *p, const char *end)
{
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
        p++;
    }
    return p;
}

/*
 * Copies a JSON string. p must point at the opening quote. out_length receives
 * the untruncated character count so a caller that must not accept a silently
 * shortened value can reject it instead. \u is rejected rather than guessed at:
 * it cannot occur in this schema.
 */
static const char *parse_string(const char *p, const char *end, char *out,
                                size_t out_size, size_t *out_length)
{
    size_t n = 0;

    if (p >= end || *p != '"') {
        return NULL;
    }
    p++;
    while (p < end && *p != '"') {
        char c = *p;

        if (c == '\\') {
            p++;
            if (p >= end) {
                return NULL;
            }
            switch (*p) {
            case '"':  c = '"';  break;
            case '\\': c = '\\'; break;
            case '/':  c = '/';  break;
            case 'b':  c = '\b'; break;
            case 'f':  c = '\f'; break;
            case 'n':  c = '\n'; break;
            case 'r':  c = '\r'; break;
            case 't':  c = '\t'; break;
            default:   return NULL;
            }
        } else if ((unsigned char)c < 0x20) {
            return NULL;  /* a raw control character is not valid JSON */
        }
        p++;
        if (out_size > 0 && n + 1 < out_size) {
            out[n] = c;
        }
        n++;
    }
    if (p >= end) {
        return NULL;
    }
    p++;
    if (out_size > 0) {
        out[n] = '\0';
    }
    if (out_length) {
        *out_length = n;
    }
    return p;
}

static const char *parse_bool(const char *p, const char *end, int *out)
{
    p = skip_ws(p, end);

    if ((size_t)(end - p) >= 4 && memcmp(p, "true", 4) == 0) {
        *out = 1;
        return p + 4;
    }
    if ((size_t)(end - p) >= 5 && memcmp(p, "false", 5) == 0) {
        *out = 0;
        return p + 5;
    }
    return NULL;
}

/* Reads a non-negative integer that fits in 32 bits. */
static const char *parse_uint(const char *p, const char *end, unsigned int *out)
{
    unsigned long long value = 0;
    int digits = 0;

    p = skip_ws(p, end);
    if (p < end && *p == '-') {
        return NULL;
    }
    while (p < end && *p >= '0' && *p <= '9') {
        value = value * 10u + (unsigned long long)(*p - '0');
        digits++;
        if (digits > 10 || value > 0xFFFFFFFFull) {
            return NULL;
        }
        p++;
    }
    if (digits == 0) {
        return NULL;
    }
    *out = (unsigned int)value;
    return p;
}

static unsigned long long gcd_ull(unsigned long long a, unsigned long long b)
{
    while (b != 0) {
        unsigned long long t = a % b;
        a = b;
        b = t;
    }
    return a;
}

/*
 * Reduces a decimal literal to an exact rational.
 *
 * The fraction is reduced because that is the form the plan generator emits:
 * Fraction("1.1") is 11/10, and a label built from 11/10 therefore reads the
 * same as the built-in table. An exponent is rejected; the schema is plain
 * decimals, and reading one as a different scale would be worse than refusing.
 */
static const char *parse_scale(const char *p, const char *end,
                               unsigned int *num, unsigned int *den)
{
    unsigned long long n = 0;
    unsigned long long d = 1;
    int digits = 0;

    p = skip_ws(p, end);
    if (p < end && *p == '-') {
        return NULL;  /* scale must be positive */
    }
    while (p < end && *p >= '0' && *p <= '9') {
        n = n * 10u + (unsigned long long)(*p - '0');
        digits++;
        if (digits > 18 || n > (unsigned long long)PIT_MOD_NUM_MAX) {
            return NULL;
        }
        p++;
    }
    if (p < end && *p == '.') {
        p++;
        while (p < end && *p >= '0' && *p <= '9') {
            n = n * 10u + (unsigned long long)(*p - '0');
            d = d * 10u;
            digits++;
            if (digits > 18 || d > (unsigned long long)PIT_MOD_DEN_MAX) {
                return NULL;
            }
            p++;
        }
    }
    if (digits == 0 || n == 0) {
        return NULL;
    }
    if (p < end && (*p == 'e' || *p == 'E')) {
        return NULL;
    }
    {
        unsigned long long g = gcd_ull(n, d);
        *num = (unsigned int)(n / g);
        *den = (unsigned int)(d / g);
    }
    return p;
}

static const char *skip_value(const char *p, const char *end, int depth)
{
    char close;
    int is_object;

    if (depth > MOD_JSON_DEPTH) {
        return NULL;
    }
    p = skip_ws(p, end);
    if (p >= end) {
        return NULL;
    }
    if (*p == '"') {
        return parse_string(p, end, NULL, 0, NULL);
    }
    if (*p == '{' || *p == '[') {
        is_object = (*p == '{');
        close = is_object ? '}' : ']';
        p = skip_ws(p + 1, end);
        if (p < end && *p == close) {
            return p + 1;
        }
        for (;;) {
            if (is_object) {
                p = skip_ws(p, end);
                if (p >= end || *p != '"') {
                    return NULL;
                }
                p = parse_string(p, end, NULL, 0, NULL);
                if (!p) {
                    return NULL;
                }
                p = skip_ws(p, end);
                if (p >= end || *p != ':') {
                    return NULL;
                }
                p++;
            }
            p = skip_value(p, end, depth + 1);
            if (!p) {
                return NULL;
            }
            p = skip_ws(p, end);
            if (p >= end) {
                return NULL;
            }
            if (*p == ',') {
                p++;
                continue;
            }
            if (*p == close) {
                return p + 1;
            }
            return NULL;
        }
    }
    if (*p == 't' || *p == 'f') {
        int ignored = 0;
        return parse_bool(p, end, &ignored);
    }
    if (*p == 'n') {
        if ((size_t)(end - p) >= 4 && memcmp(p, "null", 4) == 0) {
            return p + 4;
        }
        return NULL;
    }
    {
        const char *start = p;
        int digits = 0;

        if (p < end && *p == '-') {
            p++;
        }
        while (p < end && *p >= '0' && *p <= '9') {
            p++;
            digits++;
        }
        if (p < end && *p == '.') {
            p++;
            while (p < end && *p >= '0' && *p <= '9') {
                p++;
                digits++;
            }
        }
        if (digits == 0) {
            return NULL;
        }
        if (p < end && (*p == 'e' || *p == 'E')) {
            p++;
            if (p < end && (*p == '+' || *p == '-')) {
                p++;
            }
            if (p >= end || *p < '0' || *p > '9') {
                return NULL;
            }
            while (p < end && *p >= '0' && *p <= '9') {
                p++;
            }
        }
        return (p > start) ? p : NULL;
    }
}

/*
 * Reads the key of the next object member, leaving p at its value. Sets *done
 * when the object ended instead, and returns NULL only on malformed input.
 */
static const char *member_key(const char *p, const char *end, char *key,
                              size_t key_size, int *done)
{
    size_t length = 0;

    p = skip_ws(p, end);
    *done = 0;
    if (p >= end) {
        return NULL;
    }
    if (*p == '}') {
        *done = 1;
        return p + 1;
    }
    p = parse_string(p, end, key, key_size, &length);
    if (!p) {
        return NULL;
    }
    p = skip_ws(p, end);
    if (p >= end || *p != ':') {
        return NULL;
    }
    return skip_ws(p + 1, end);
}

/*
 * Consumes the separator after a member value. Sets *done at the closing brace
 * or bracket and returns the position of the next key or element.
 */
static const char *separator(const char *p, const char *end, char close,
                             int *done)
{
    p = skip_ws(p, end);
    if (p >= end) {
        return NULL;
    }
    if (*p == ',') {
        p = skip_ws(p + 1, end);
        /*
         * A comma must be followed by another member. Accepting "}" or "]"
         * here would make a trailing comma legal, which JSON forbids, so a typo
         * at the end of a profile is reported instead of silently ignored.
         */
        if (p >= end || *p == close || *p == ',') {
            return NULL;
        }
        return p;
    }
    if (*p == close) {
        *done = 1;
        return p + 1;
    }
    return NULL;
}

static const char *object_open(const char *p, const char *end, char *error,
                               size_t error_size, const char *what)
{
    p = skip_ws(p, end);
    if (p >= end || *p != '{') {
        snprintf(error, error_size, "%s is not an object", what);
        return NULL;
    }
    return skip_ws(p + 1, end);
}

/* ------------------------------------------------------------ profile input */

/*
 * Reads one transform. Returns 1 when it is enabled and stored, 0 when it is
 * valid but disabled, and -1 when it is invalid. after receives the position
 * just past the closing brace so the caller can continue the list.
 */
static int parse_transform(const char *p, const char *end,
                           pit_mod_transform *out, const char **after,
                           char *error, size_t error_size)
{
    int done = 0;
    int enabled = 1;
    int have_field = 0;
    int have_scale = 0;
    size_t length = 0;
    pit_mod_transform t;

    memset(&t, 0, sizeof(t));
    t.min_value = 0u;
    t.max_value = MOD_U16_MAX;

    p = object_open(p, end, error, error_size, "transform");
    if (!p) {
        return -1;
    }
    *after = p;

    for (;;) {
        char key[MOD_KEY_MAX];
        const char *next;

        p = member_key(p, end, key, sizeof(key), &done);
        if (!p) {
            snprintf(error, error_size, "transform has a bad member");
            return -1;
        }
        *after = p;
        if (done) {
            break;
        }

        if (strcmp(key, "field") == 0) {
            length = 0;
            next = parse_string(p, end, t.field, sizeof(t.field), &length);
            if (!next || length == 0 || length > PIT_MOD_FIELD_MAX) {
                snprintf(error, error_size,
                         "transform 'field' must be 1..%u characters",
                         (unsigned int)PIT_MOD_FIELD_MAX);
                return -1;
            }
            have_field = 1;
        } else if (strcmp(key, "scale") == 0) {
            next = parse_scale(p, end, &t.num, &t.den);
            if (!next) {
                snprintf(error, error_size,
                         "transform 'scale' must be a positive decimal");
                return -1;
            }
            have_scale = 1;
        } else if (strcmp(key, "min") == 0) {
            next = parse_uint(p, end, &t.min_value);
            if (!next) {
                snprintf(error, error_size,
                         "transform 'min' must be a non-negative integer");
                return -1;
            }
        } else if (strcmp(key, "max") == 0) {
            next = parse_uint(p, end, &t.max_value);
            if (!next) {
                snprintf(error, error_size,
                         "transform 'max' must be a non-negative integer");
                return -1;
            }
        } else if (strcmp(key, "enabled") == 0) {
            next = parse_bool(p, end, &enabled);
            if (!next) {
                snprintf(error, error_size,
                         "transform 'enabled' must be true or false");
                return -1;
            }
        } else {
            /* An unrecognised member costs nothing to ignore. */
            next = skip_value(p, end, 1);
            if (!next) {
                snprintf(error, error_size, "transform member '%s' is invalid",
                         key);
                return -1;
            }
        }

        p = separator(next, end, '}', &done);
        if (!p) {
            snprintf(error, error_size, "transform is truncated");
            return -1;
        }
        *after = p;
        if (done) {
            break;
        }
    }

    if (!have_field) {
        snprintf(error, error_size, "transform has no 'field'");
        return -1;
    }
    if (!have_scale) {
        snprintf(error, error_size, "transform '%s' has no 'scale'", t.field);
        return -1;
    }
    if (t.min_value > t.max_value) {
        snprintf(error, error_size, "transform '%s' has min greater than max",
                 t.field);
        return -1;
    }
    if (t.min_value > MOD_U16_MAX) {
        t.min_value = MOD_U16_MAX;
    }
    if (t.max_value > MOD_U16_MAX) {
        t.max_value = MOD_U16_MAX;
    }
    /*
     * A disabled transform is kept, like the Java parser, so the MODS list can
     * show it greyed out. Whether it is applied is decided at resolve time, not
     * by dropping it here.
     */
    t.enabled = enabled;
    *out = t;
    return 1;
}

static const char *parse_transforms(const char *p, const char *end,
                                    pit_mod_profile *profile, char *error,
                                    size_t error_size)
{
    int done = 0;

    p = skip_ws(p, end);
    if (p >= end || *p != '[') {
        snprintf(error, error_size, "'transforms' must be a list");
        return NULL;
    }
    p = skip_ws(p + 1, end);
    if (p < end && *p == ']') {
        snprintf(error, error_size, "profile has an empty 'transforms' list");
        return NULL;
    }

    for (;;) {
        const char *after = NULL;

        if (profile->transform_count >= PIT_MOD_TRANSFORM_MAX) {
            snprintf(error, error_size, "profile has more than %u transforms",
                     (unsigned int)PIT_MOD_TRANSFORM_MAX);
            return NULL;
        }
        if (parse_transform(p, end, &profile->transforms[profile->transform_count],
                            &after, error, error_size) < 0) {
            return NULL;
        }
        profile->transform_count++;
        p = separator(after, end, ']', &done);
        if (!p) {
            snprintf(error, error_size, "'transforms' is truncated");
            return NULL;
        }
        if (done) {
            return p;
        }
    }
}

static const char *parse_applies_to(const char *p, const char *end,
                                    pit_mod_profile *profile, char *error,
                                    size_t error_size)
{
    int done = 0;

    p = object_open(p, end, error, error_size, "'applies_to'");
    if (!p) {
        return NULL;
    }
    for (;;) {
        char key[MOD_KEY_MAX];
        const char *next;

        p = member_key(p, end, key, sizeof(key), &done);
        if (!p) {
            snprintf(error, error_size, "'applies_to' has a bad member");
            return NULL;
        }
        if (done) {
            return p;
        }
        if (strcmp(key, "version") == 0) {
            next = parse_string(p, end, profile->applies_to,
                                sizeof(profile->applies_to), NULL);
        } else {
            next = skip_value(p, end, 1);
        }
        if (!next) {
            snprintf(error, error_size, "'applies_to' is malformed");
            return NULL;
        }
        p = separator(next, end, '}', &done);
        if (!p) {
            snprintf(error, error_size, "'applies_to' is truncated");
            return NULL;
        }
        if (done) {
            return p;
        }
    }
}

static char *read_text_file(const char *path)
{
    FILE *fp;
    char *text;
    long size;
    size_t read_bytes;

    fp = fopen(path, "rb");
    if (!fp) {
        return NULL;
    }
    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    size = ftell(fp);
    /* An empty or implausibly large file is not a profile. */
    if (size <= 0 || (unsigned long)size > MOD_FILE_MAX) {
        fclose(fp);
        return NULL;
    }
    rewind(fp);
    text = (char *)malloc((size_t)size + 1u);
    if (!text) {
        fclose(fp);
        return NULL;
    }
    read_bytes = fread(text, 1, (size_t)size, fp);
    fclose(fp);
    if (read_bytes != (size_t)size) {
        free(text);
        return NULL;
    }
    text[size] = '\0';
    return text;
}

int pit_mods_load(const char *path, pit_mod_profile *out, char *error,
                  size_t error_size)
{
    char *text;
    const char *p;
    const char *end;
    char schema[MOD_KEY_MAX];
    int done = 0;
    int have_schema = 0;
    int have_id = 0;

    if (error && error_size > 0) {
        error[0] = '\0';
    }
    if (!path || !out) {
        if (error && error_size > 0) {
            snprintf(error, error_size, "no profile path");
        }
        return -1;
    }
    memset(out, 0, sizeof(*out));
    memset(schema, 0, sizeof(schema));

    text = read_text_file(path);
    if (!text) {
        snprintf(error, error_size, "cannot read %s", path);
        return -1;
    }
    end = text + strlen(text);
    p = object_open(text, end, error, error_size, "profile");
    if (!p) {
        free(text);
        return -1;
    }

    for (;;) {
        char key[MOD_KEY_MAX];
        const char *next;

        p = member_key(p, end, key, sizeof(key), &done);
        if (!p) {
            snprintf(error, error_size, "profile has a bad member");
            free(text);
            return -1;
        }
        if (done) {
            break;
        }

        if (strcmp(key, "schema") == 0) {
            next = parse_string(p, end, schema, sizeof(schema), NULL);
            have_schema = 1;
        } else if (strcmp(key, "id") == 0) {
            /*
             * The length is reported rather than truncated. The id is the
             * profile's identity: --mod, the MODS list and the output name all
             * key off it, so two profiles whose ids truncate to the same string
             * would be indistinguishable after loading.
             */
            size_t id_length = 0;

            next = parse_string(p, end, out->id, sizeof(out->id), &id_length);
            if (next) {
                if (id_length == 0 || id_length > PIT_MOD_ID_MAX) {
                    snprintf(error, error_size, "'id' must be 1..%u characters",
                             (unsigned int)PIT_MOD_ID_MAX);
                    return -1;
                }
                have_id = 1;
            }
        } else if (strcmp(key, "name") == 0) {
            next = parse_string(p, end, out->name, sizeof(out->name), NULL);
        } else if (strcmp(key, "version") == 0) {
            next = parse_string(p, end, out->version, sizeof(out->version), NULL);
        } else if (strcmp(key, "description") == 0) {
            next = parse_string(p, end, out->description,
                                sizeof(out->description), NULL);
        } else if (strcmp(key, "applies_to") == 0) {
            next = parse_applies_to(p, end, out, error, error_size);
        } else if (strcmp(key, "transforms") == 0) {
            next = parse_transforms(p, end, out, error, error_size);
            if (!next) {
                free(text);
                return -1;
            }
        } else {
            next = skip_value(p, end, 1);
        }

        if (!next) {
            snprintf(error, error_size, "profile member '%s' is invalid", key);
            free(text);
            return -1;
        }
        p = separator(next, end, '}', &done);
        if (!p) {
            snprintf(error, error_size, "profile is truncated");
            free(text);
            return -1;
        }
        if (done) {
            break;
        }
    }

    /*
     * The profile object must be the whole file. Accepting trailing text would
     * let two concatenated objects parse as one profile, which reads as a
     * silent data error rather than a malformed file.
     */
    p = skip_ws(p, end);
    if (p != end) {
        snprintf(error, error_size, "trailing text after the profile object");
        free(text);
        return -1;
    }

    free(text);

    if (!have_schema || strcmp(schema, PIT_MOD_SCHEMA) != 0) {
        snprintf(error, error_size, "schema must be '%s'", PIT_MOD_SCHEMA);
        return -1;
    }
    if (!have_id) {
        snprintf(error, error_size, "profile has no 'id'");
        return -1;
    }
    if (out->name[0] == '\0') {
        snprintf(out->name, sizeof(out->name), "%s", out->id);
    }
    {
        /*
         * Every transform is retained, so "no enabled transforms" has to be
         * checked by looking at the flags rather than at the count.
         */
        unsigned int i;
        int enabled = 0;

        for (i = 0; i < out->transform_count; i++) {
            if (out->transforms[i].enabled) {
                enabled = 1;
                break;
            }
        }
        if (!enabled) {
            snprintf(error, error_size, "profile has no enabled transforms");
            return -1;
        }
    }
    {
        /*
         * Two transforms naming one field are refused. Applying both in order
         * would scale the value twice, which reads as a mistake in the profile
         * rather than an intent this launcher can express, and it would make the
         * written bytes depend on array order.
         */
        unsigned int i;
        unsigned int j;

        for (i = 0; i < out->transform_count; i++) {
            for (j = i + 1; j < out->transform_count; j++) {
                if (strcmp(out->transforms[i].field, out->transforms[j].field) == 0) {
                    snprintf(error, error_size,
                             "transforms %u and %u both name '%s'", i + 1, j + 1,
                             out->transforms[i].field);
                    return -1;
                }
            }
        }
    }
    return 0;
}

/* ------------------------------------------------------------------- scale */

unsigned int pit_mods_scale(unsigned int value, unsigned int num,
                            unsigned int den, unsigned int min_value,
                            unsigned int max_value)
{
    unsigned long long scaled;

    if (den == 0u) {
        return (value < min_value) ? min_value : value;
    }
    /*
     * Half-up rounding on exact integers: adding den/2 before the division
     * carries an exact .5 upwards. den/2 truncates, which is right because an
     * odd denominator can never produce an exact half.
     */
    scaled = ((unsigned long long)value * (unsigned long long)num +
              (unsigned long long)(den / 2u)) / (unsigned long long)den;
    if (scaled < (unsigned long long)min_value) {
        return min_value;
    }
    if (scaled > (unsigned long long)max_value) {
        return max_value;
    }
    return (unsigned int)scaled;
}

/* ------------------------------------------------------------- directories */

int pit_mods_profile_path(char *path, size_t path_size, const char *root,
                          const char *name)
{
    size_t need;

    if (!path || !root || !name || path_size == 0) {
        return -1;
    }
    need = strlen(root) + strlen(name) + strlen("/profile.json") + 2u;
    if (need > path_size) {
        return -1;
    }
    snprintf(path, path_size, "%s/%s/profile.json", root, name);
    return 0;
}

static int is_directory(const char *path)
{
    struct stat st;

    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode) ? 1 : 0;
}

static int file_exists(const char *path)
{
    struct stat st;

    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISREG(st.st_mode) ? 1 : 0;
}

typedef struct {
    char names[PIT_MOD_MAX][PIT_MOD_DIR_MAX];
    unsigned int count;
} mod_dirlist;

static void dirlist_add(mod_dirlist *list, const char *name)
{
    size_t length = strlen(name);

    if (list->count >= PIT_MOD_MAX) {
        return;
    }
    if (length == 0 || length >= PIT_MOD_DIR_MAX || name[0] == '.') {
        return;
    }
    snprintf(list->names[list->count], PIT_MOD_DIR_MAX, "%s", name);
    list->count++;
}

static void dirlist_sort(mod_dirlist *list)
{
    unsigned int i;

    for (i = 1; i < list->count; i++) {
        char tmp[PIT_MOD_DIR_MAX];
        unsigned int j = i;

        memcpy(tmp, list->names[i], sizeof(tmp));
        while (j > 0 && strcmp(list->names[j - 1], tmp) > 0) {
            memcpy(list->names[j], list->names[j - 1], PIT_MOD_DIR_MAX);
            j--;
        }
        memcpy(list->names[j], tmp, sizeof(tmp));
    }
}

static void list_subdirs(const char *root, mod_dirlist *list)
{
    list->count = 0;
    if (!root || root[0] == '\0' || !is_directory(root)) {
        return;
    }
#if defined(_WIN32)
    {
        char pattern[MOD_PATH_MAX];
        WIN32_FIND_DATAA data;
        HANDLE handle;

        snprintf(pattern, sizeof(pattern), "%s\\*", root);
        handle = FindFirstFileA(pattern, &data);
        if (handle == INVALID_HANDLE_VALUE) {
            return;
        }
        do {
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                dirlist_add(list, data.cFileName);
            }
        } while (FindNextFileA(handle, &data));
        FindClose(handle);
    }
#else
    {
        DIR *dir = opendir(root);
        struct dirent *entry;

        if (!dir) {
            return;
        }
        while ((entry = readdir(dir)) != NULL) {
            char child[MOD_PATH_MAX];

            if (entry->d_name[0] == '.') {
                continue;
            }
            snprintf(child, sizeof(child), "%s/%s", root, entry->d_name);
            if (is_directory(child)) {
                dirlist_add(list, entry->d_name);
            }
        }
        closedir(dir);
    }
#endif
}

static void catalog_sort(pit_mod_catalog *catalog)
{
    unsigned int i;

    for (i = 1; i < catalog->count; i++) {
        pit_mod_profile tmp = catalog->profiles[i];
        unsigned int j = i;

        while (j > 0) {
            int order = strcmp(catalog->profiles[j - 1].id, tmp.id);

            if (order == 0) {
                order = strcmp(catalog->profiles[j - 1].dir, tmp.dir);
            }
            if (order <= 0) {
                break;
            }
            catalog->profiles[j] = catalog->profiles[j - 1];
            j--;
        }
        catalog->profiles[j] = tmp;
    }
}

int pit_mods_scan(const char *root, pit_mod_catalog *out, char *error,
                  size_t error_size, unsigned int *skipped)
{
    mod_dirlist list;
    char path[MOD_PATH_MAX];
    char reason[192];
    unsigned int i;
    unsigned int bad = 0;

    if (error && error_size > 0) {
        error[0] = '\0';
    }
    if (!out) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    if (skipped) {
        *skipped = 0;
    }
    if (!root || root[0] == '\0') {
        snprintf(error, error_size, "no mods directory");
        return -1;
    }

    list_subdirs(root, &list);
    dirlist_sort(&list);

    for (i = 0; i < list.count; i++) {
        pit_mod_profile profile;

        if (pit_mods_profile_path(path, sizeof(path), root, list.names[i]) != 0) {
            continue;
        }
        if (!file_exists(path)) {
            continue;
        }
        if (pit_mods_load(path, &profile, reason, sizeof(reason)) != 0) {
            /*
             * One unusable mod must not hide the others, so a bad profile is
             * counted and skipped rather than ending the scan. The first
             * failure is the one reported, because a later successful load
             * would otherwise clear the reason.
             */
            bad++;
            if (error && error_size > 0 && error[0] == '\0') {
                snprintf(error, error_size, "%s: %s", list.names[i], reason);
            }
            continue;
        }
        if (out->count >= PIT_MOD_MAX) {
            break;
        }
        snprintf(profile.dir, sizeof(profile.dir), "%s", list.names[i]);
        out->profiles[out->count] = profile;
        out->count++;
    }

    catalog_sort(out);
    if (skipped) {
        *skipped = bad;
    }
    return (int)out->count;
}

const pit_mod_profile *pit_mods_find(const pit_mod_catalog *catalog,
                                     const char *id)
{
    unsigned int i;

    if (!catalog || !id || id[0] == '\0') {
        return NULL;
    }
    for (i = 0; i < catalog->count; i++) {
        if (strcmp(catalog->profiles[i].id, id) == 0) {
            return &catalog->profiles[i];
        }
    }
    return NULL;
}

int pit_mods_root_exists(const char *root)
{
    pit_mod_catalog catalog;
    char error[128];
    unsigned int skipped = 0;

    if (!root || root[0] == '\0') {
        return 0;
    }
    return pit_mods_scan(root, &catalog, error, sizeof(error), &skipped) > 0;
}