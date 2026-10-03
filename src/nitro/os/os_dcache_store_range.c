/*
 * Data cache writeback range (ARM9 resident, 0x02039FF4-0x0203A010).
 *
 * Cleans every data cache line covering [address, address + size) back to the
 * point of unification. Lines are visited in 32-byte steps starting from the
 * first line boundary at or below the requested address.
 */

#include <nitro.h>

void DC_StoreRange(const void *address, u32 size)
{
    u32 end = size + (u32)address;
    u32 line = (u32)address & ~0x1Fu;

    do {
        /* CP15 maintenance opc7/CRn7/CRm10/opc1=1: clean D-cache by MVA to PoU. */
        asm { mcr p15, 0, line, c7, c10, 1 }
        line += 0x20;
    } while ((s32)line < (s32)end);
}