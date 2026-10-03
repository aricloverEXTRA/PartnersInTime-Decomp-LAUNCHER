/*
 * Lock release (ARM9 resident, unlinked draft).
 *
 * Unlinked draft. Gap documented in source.
 */

#include <nitro.h>

﻿/*
 * Lock release (ARM9 resident, unlinked draft).
 *
 * Clears the bit for the given lock index in the appropriate bitmap.
 * Indexes 0x40-0x5F use the first bitmap; 0x60-0x7F use the second.
 */
#include <nitro.h>
void OS_ReleaseLockID(u32 id)
{
    u32 *bitmap;
    u32 bit;
    asm {
        cmp id, #0x60
        addpl bitmap, =0x027FF7B4
        addpl id, id, #-0x60
        submi id, id, #0x40
        addmi bitmap, =0x027FF7B0
        mov bit, #0x80000000
        lsr bit, bit, id
        ldr id, [bitmap]
        orr id, id, bit
        str id, [bitmap]
        bx lr
    }
}