/*
 * Halfword-granular block clear (ARM9 resident, unlinked draft).
 *
 * The save-menu and scene loaders blank fixed-size regions that are already
 * halfword aligned, so the fill runs in 16-bit stores.
 *
 * The body already matches native instruction for instruction, but both a
 * "while" and a "for" formulation make MWCC hoist the first comparison into
 * "cmp size, #0 / bxeq lr", adding 8 bytes that native does not have; a
 * "do/while" moves the test below the guarded body instead. Native has neither,
 * and nothing in this source explains why the hoist is suppressed. Revisit when
 * that compiler rule is known.
 */

#include <nitro.h>

void MIi_CpuClear16(u16 value, void *destination, u32 size)
{
    u8 *dst = (u8 *)destination;
    u32 offset;

    for (offset = 0; offset < size; offset += 2)
        *(u16 *)(dst + offset) = value;
}