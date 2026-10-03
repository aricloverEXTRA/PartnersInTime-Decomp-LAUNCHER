/*
 * DTCM base address query (ARM9 resident, unlinked draft).
 *
 * Unlinked draft. Gap documented in source.
 */

#include <nitro.h>

u32 OS_GetDTCMAddress(void)
{
    volatile u8 *block = (volatile u8 *)0x027FFC80;
    return *(volatile u64 *)(block + 0x68);
}