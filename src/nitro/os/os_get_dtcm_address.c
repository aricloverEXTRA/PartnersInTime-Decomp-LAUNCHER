/*
 * DTCM base address query (ARM9 resident, unlinked draft).
 *
 * Unlinked draft. Gap documented in source.
 */

#include <nitro.h>

﻿/*
 * DTCM base address query (ARM9 resident, unlinked draft).
 *
 * The ARM946E-S region register 1 holds both the DTCM size and its base, so
 * the size field has to be masked off to recover the address itself.
 *
 * Unlinked draft, native at unlinked draft (20 bytes). Size matches, but MWCC
 * materialises the 0xFFFFF000 mask as mov+rsb and reads CP15 into r1, where
 * native loads the mask from a literal pool and keeps the read result in r0.
 * Revisit once the literal-pool form is understood.
 */
#include <nitro.h>
u32 OS_GetDTCMAddress(void)
{
    u32 region;
    u32 size_mask = 0xFFFFF000;
    asm { mrc p15, 0, region, c9, c1, 0 }
    return region & size_mask;
}