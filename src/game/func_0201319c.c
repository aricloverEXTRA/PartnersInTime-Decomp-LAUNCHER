/*
 * Record table lookup (ARM9 resident, 0x0201319C-0x020131B0).
 *
 * Reaches the 0x14-stride table through the record pointer's field at offset
 * 0xAC and returns the word stored four bytes into entry `index`.
 */

#include <nitro.h>

u32 func_0201319c(u8 *record, u32 index)
{
    u8 *table = *(u8 **)(record + 0xAC);

    return *(u32 *)(void *)(table + index * 0x14 + 4);
}