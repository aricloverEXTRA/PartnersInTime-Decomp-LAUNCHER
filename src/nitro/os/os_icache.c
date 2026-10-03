/*
 * Instruction cache invalidation (ARM9 resident, 0x0203A040-0x0203A04C).
 *
 * Drops every line of the instruction cache.
 */

#include <nitro.h>

void IC_InvalidateAll(void)
{
    u32 zero = 0;
    /* CP15 maintenance opc7/CRn7/CRm5/opc1=0: invalidate instruction cache all. */
    asm { mcr p15, 0, zero, c7, c5, 0 }
}