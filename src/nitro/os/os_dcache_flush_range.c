/*
 * Data cache writeback and invalidate range (ARM9 resident, unlinked draft).
 *
 * Unlinked draft. Gap documented in source.
 */

#include <nitro.h>

void DC_FlushRange(const void *address, u32 size)
{
    u32 end = size + (u32)address;
    u32 line = (u32)address & ~0x1Fu;
    u32 zero = 0;

    asm { mov r12, #0 }
    do {
        asm { mcr p15, 0, r12, c7, c10, 4 }
        asm { mcr p15, 0, line, c7, c14, 1 }
        line += 0x20;
    } while ((s32)line < (s32)end);
}