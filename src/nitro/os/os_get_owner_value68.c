/*
 * RTC status/sync word query (ARM9 resident, 0x0203AFE8-0x0203AFFC).
 *
 * Reads the 64-bit word the RTC driver reserves in the owner-info block and
 * hands it to the boot-time session setup.
 */

#include <nitro.h>

u64 OS_GetOwnerValue68(void)
{
    volatile u8 *block = (volatile u8 *)0x027FFC80;

    return *(volatile u64 *)(block + 0x68);
}