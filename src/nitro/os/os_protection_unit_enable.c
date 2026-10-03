/*
 * MPU enable (ARM9 resident, 0x0203A4B0-0x0203A4C0).
 *
 * Sets the protection-unit bit in the CP15 control register so the region
 * descriptors programmed through OS_SetProtectionRegion1/2 start enforcing.
 */

#include <nitro.h>

void OS_EnableProtectionUnit(void)
{
    u32 control;
    asm {
        mrc p15, 0, control, c1, c0, 0
        orr control, control, #1
        mcr p15, 0, control, c1, c0, 0
    }
}
