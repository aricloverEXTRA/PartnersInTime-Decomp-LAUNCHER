/*
 * Halfword-granular block copy (ARM9 resident, unlinked draft).
 *
 * Unlinked draft. Gap documented in source.
 */

#include <nitro.h>

void MIi_CpuCopy16(const void *source, void *destination, u32 size)
{
    const u8 *src = (const u8 *)source;
    u8 *dst = (u8 *)destination;
    u32 offset = 0;

    while (offset < size) {
        *(u16 *)(dst + offset) = *(const u16 *)(src + offset);
        offset += 2;
    }
}