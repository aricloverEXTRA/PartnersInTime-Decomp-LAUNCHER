/*
 * Halfword-granular block clear (ARM9 resident, unlinked draft).
 *
 * Unlinked draft. Gap documented in source.
 */

#include <nitro.h>

void MIi_CpuClear16(u16 value, void *destination, u32 size)
{
    u8 *dst = (u8 *)destination;
    u32 offset = 0;

    while (offset < size) {
        *(u16 *)(dst + offset) = value;
        offset += 2;
    }
}