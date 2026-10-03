/*
 * Data cache invalidation range (ARM9 resident, 0x02039FD8-0x02039FF4).
 *
 * Drops every data cache line covering [address, address + size) without
 * writing it back, so it has to be preceded by DC_StoreRange whenever the
 * lines are dirty.
 */

#include <nitro.h>

void DC_InvalidateRange(const void *address, u32 size)
{
    u32 end = size + (u32)address;
    u32 line = (u32)address & ~0x1Fu;

    do {
        /* CP15 maintenance opc7/CRn7/CRm6/opc1=1: invalidate data cache by MVA. */
        asm { mcr p15, 0, line, c7, c6, 1 }
        line += 0x20;
    } while ((s32)line < (s32)end);
}