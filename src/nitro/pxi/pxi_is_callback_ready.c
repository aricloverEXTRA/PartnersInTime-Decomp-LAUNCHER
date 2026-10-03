/*
 * PXI callback ready check (ARM9 resident, unlinked draft).
 *
 * Unlinked draft. Gap documented in source.
 */

#include <nitro.h>

﻿/*
 * PXI callback ready check (ARM9 resident, unlinked draft).
 *
 * Checks a bit in the PXI FIFO callback table. The table base is at
 * 0x0207FC00; the index selects a word at offset +0x388, then a
 * single-bit mask is tested.
 *
 * The body already matches native instruction for instruction, but MWCC
 * materialises the table base as a series of adds instead of a literal
 * pool load, adding 4 bytes. Revisit once the literal-pool form is known.
 */
#include <nitro.h>
u32 PXI_IsCallbackReady(u32 bit, u32 index)
{
    u32 *table = (u32 *)0x0207FC00;
    u32 mask = 1u << bit;
    u32 word = table[index + 0x388 / 4];
    return (word & mask) ? 1 : 0;
}