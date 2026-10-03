/*
 * Protection region 2 readback (ARM9 resident, 0x0203A4E0-0x0203A4E8).
 *
 * Returns the descriptor currently programmed into region 2.
 */

#include <nitro.h>

u32 OS_GetProtectionRegion2(void)
{
    u32 value;
    asm { mrc p15, 0, value, c6, c2, 0 }
    return value;
}
