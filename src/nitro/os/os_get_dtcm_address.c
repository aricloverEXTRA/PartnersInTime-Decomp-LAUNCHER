/*
 * DTCM base address query (ARM9 resident, unlinked draft).
 *
 * The ARM946E-S region register 1 holds both the DTCM size and its base, so
 * the size field has to be masked off to recover the address itself.
 *
 * Unlinked draft, native at 0x0203A49C (20 bytes). Size matches, but MWCC
 * materialises the 0xFFFFF000 mask as mov+rsb and reads CP15 into r1, where
 * native loads the mask from a literal pool and keeps the read result in r0.
 * Revisit once the literal-pool form is understood.
 */

#include <nitro.h>

u32 OS_GetDTCMAddress(void)
{
    u32 region;
    u32 size_mask = 0xFFFFF000;

    /* CP15 opc1=0/CRn9/CRm1/opc2=0: read DTCM region 1 (base | size). */
    asm { mrc p15, 0, region, c9, c1, 0 }
    return region & size_mask;
}