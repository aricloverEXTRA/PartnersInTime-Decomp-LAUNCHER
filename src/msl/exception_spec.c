/* Match a thrown type against the packed exception-specification type list. */
#include "exception_search_internal.h"
/* Native type-name matcher; its implementation remains unreconstructed. */
extern int func_02048614(const u8 *, const u8 *, s32 *);

int MSL_MatchesExceptionSpecification(const u8 *type, const MslExceptionSpec *spec)
{
    const u8 *input = spec->types;
    u32 index;
    struct {const u8 *type; s32 adjustment;} match;
    for (index = 0; index < spec->count; ++index, input += 4) {
        match.type = (const u8 *)((u32)input[0] | (u32)input[1] << 8 | (u32)input[2] << 16 | (u32)input[3] << 24);
        if (func_02048614(type, match.type, &match.adjustment)) return 1;
    }
    return 0;
}
