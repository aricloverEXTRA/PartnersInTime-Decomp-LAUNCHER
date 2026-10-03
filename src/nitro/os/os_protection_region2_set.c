/*
 * Protection region 2 (ARM9 resident, 0x0203A4D8-0x0203A4E0).
 *
 * Second half of the CP15 MPU region pair; os_arena.c supplies the encoded
 * region for the DTCM-adjacent block.
 */

#include <nitro.h>

void OS_SetProtectionRegion2(u32 value)
{
    /* CP15 MPU region 2 register, CRn6/CRm2/opc1=0. */
    asm { mcr p15, 0, value, c6, c2, 0 }
}
