/*
 * PXI callback ready check (ARM9 resident, unlinked draft).
 *
 * Unlinked draft. Gap documented in source.
 */

#include <nitro.h>

u32 PXI_IsCallbackReady(u32 bit, u32 index)
{
    u32 *table = (u32 *)0x0207FC00;
    u32 mask = 1u << bit;
    u32 word = table[index + 0x388 / 4];
    return (word & mask) ? 1 : 0;
}