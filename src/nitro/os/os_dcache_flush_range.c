/*
 * Data cache writeback and invalidate range (ARM9 resident, unlinked draft).
 *
 * Unlinked draft. Gap documented in source.
 */

#include <nitro.h>

﻿/*
 * Data cache writeback and invalidate range (ARM9 resident, unlinked draft).
 *
 * Drains this core's write buffer before invalidating each line, so a stale
 * buffered word cannot be written back on top of the invalidate.
 *
 * Unlinked draft, native at unlinked draft (36 bytes). Size matches, but the drain
 * operand lands in r2 instead of the native r12: the loop needs r0 and r1 for
 * the line and end addresses, and the allocator offers no source-level slot
 * that yields r12. Revisit if a compiler rule for scratch-register asm operands
 * is established.
 */
#include <nitro.h>
void DC_FlushRange(const void *address, u32 size)
{
    u32 end = size + (u32)address;
    u32 line = (u32)address & ~0x1Fu;
    asm { mov r12, #0 }
    do {
        asm { mcr p15, 0, r12, c7, c10, 4 }
        asm { mcr p15, 0, line, c7, c14, 1 }
        line += 0x20;
    } while ((s32)line < (s32)end);
}