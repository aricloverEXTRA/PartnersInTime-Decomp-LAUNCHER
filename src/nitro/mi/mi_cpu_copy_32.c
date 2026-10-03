/*
 * Word-granular block copy (ARM9 resident, unlinked draft).
 *
 * Unlinked draft. Gap documented in source.
 */

#include <nitro.h>

void func_0203b7b4(const void *source, void *destination, u32 size)
{
    const u32 *src = (const u32 *)source;
    u32 *dst = (u32 *)destination;
    u32 *end = dst + size;

    while (dst < end) {
        *dst++ = *src++;
    }
}