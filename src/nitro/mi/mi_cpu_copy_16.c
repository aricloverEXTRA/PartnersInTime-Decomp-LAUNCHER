/*
 * Halfword-granular block copy (ARM9 resident, unlinked draft).
 *
 * The GX texture and palette loaders move 16-bit units, so this keeps the
 * transfer halfword-aligned rather than falling back to a byte loop.
 *
 * The body already matches native instruction for instruction, but both a
 * "while" and a "for" formulation make MWCC hoist the first comparison into
 * "cmp size, #0 / bxeq lr", adding 8 bytes that native does not have; a
 * "do/while" moves the test below the guarded body instead. Native has neither,
 * and nothing in this source explains why the hoist is suppressed. Revisit when
 * that compiler rule is known.
 */

#include <nitro.h>

void MIi_CpuCopy16(const void *source, void *destination, u32 size)
{
    const u8 *src = (const u8 *)source;
    u8 *dst = (u8 *)destination;
    u32 offset;

    for (offset = 0; offset < size; offset += 2)
        *(u16 *)(dst + offset) = *(const u16 *)(src + offset);
}