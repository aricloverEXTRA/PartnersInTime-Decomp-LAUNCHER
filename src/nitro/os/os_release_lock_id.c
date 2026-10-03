/*
 * Lock release (ARM9 resident, unlinked draft).
 *
 * Unlinked draft. Gap documented in source.
 */

#include <nitro.h>

void OS_ReleaseLockID(u32 id)
{
    u32 *bitmap1 = (u32 *)0x027FF7B0;
    u32 *bitmap2 = (u32 *)0x027FF7B4;
    u32 bit;

    if (id >= 0x60) {
        bit = 1u << (id - 0x60);
        *bitmap2 |= bit;
    } else {
        bit = 1u << (id - 0x40);
        *bitmap1 |= bit;
    }
}