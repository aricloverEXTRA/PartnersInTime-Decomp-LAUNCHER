/*
 * Data cache invalidation (ARM9 resident, 0x02039F6C-0x02039F78).
 *
 * Drops every line of the data cache. The SDK takes no argument because each
 * ARM9 core only ever invalidates its own cache.
 */

#include <nitro.h>

void DC_InvalidateAll(void)
{
    u32 zero = 0;
    /* CP15 maintenance opc7/CRn7/CRm6/opc1=0: invalidate data cache all. */
    asm { mcr p15, 0, zero, c7, c6, 0 }
}