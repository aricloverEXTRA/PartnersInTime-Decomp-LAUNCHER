/*
 * Protection region 1 (ARM9 resident, 0x0203A4D0-0x0203A4D8).
 *
 * Writes a packed region descriptor straight into the first MPU region slot;
 * os_arena.c supplies the encoded region.
 */

#include <nitro.h>

void OS_SetProtectionRegion1(u32 value)
{
    /* CP15 MPU region 1 register, CRn6/CRm1/opc1=0. */
    asm { mcr p15, 0, value, c6, c1, 0 }
}