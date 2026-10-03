/*
 * Instruction cache invalidation range (ARM9 resident, 0x0203A04C-0x0203A068).
 *
 * Drops every instruction cache line covering [address, address + size), which
 * is what the overlays and freshly loaded code need before they run.
 */

#include <nitro.h>

void IC_InvalidateRange(const void *address, u32 size)
{
    u32 end = size + (u32)address;
    u32 line = (u32)address & ~0x1Fu;

    do {
        /* CP15 maintenance opc7/CRn7/CRm5/opc1=1: invalidate instruction cache by MVA. */
        asm { mcr p15, 0, line, c7, c5, 1 }
        line += 0x20;
    } while ((s32)line < (s32)end);
}