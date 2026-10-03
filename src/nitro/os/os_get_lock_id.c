/*
 * Lock allocator (ARM9 resident, unlinked draft).
 *
 * Unlinked draft. Gap documented in source.
 */

#include <nitro.h>

﻿/*
 * Lock allocator (ARM9 resident, unlinked draft).
 *
 * Finds a free bit in the lock bitmap using CLZ, marks it allocated,
 * and returns the lock index + 0x40 (or 0x60 if wrap).
 */
#include <nitro.h>
u32 OS_GetLockID(void)
{
    u32 *bitmap1 = (u32 *)0x027FF7B0;
    u32 *bitmap2 = (u32 *)0x027FF7B4;
    u32 val, idx, mask;
    asm { mrs r0, CPSR }  /* dummy to test asm - will remove */
    val = *bitmap1;
    asm { clz idx, val }
    if (idx != 0x20) {
        mask = 0x80000000 >> idx;
        *bitmap1 &= ~mask;
        return 0x40 + idx;
    }
    val = *bitmap2;
    asm { clz idx, val }
    if (idx == 0x20) return 0;
    mask = 0x80000000 >> idx;
    *bitmap2 &= ~mask;
    return 0x60 + idx;
}