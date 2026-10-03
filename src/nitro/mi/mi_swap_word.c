/*
 * Atomic word exchange (ARM9 resident, 0x0203BA9C-0x0203BAA4).
 *
 * SWP claims the word in one bus transaction and returns what was there, which
 * is how os_lock.c arbitrates a lock word between the two CPUs.
 */

#include <nitro.h>

u32 MI_SwapWord(u32 value, volatile u32 *address)
{
    asm { swp value, value, [address] }
    return value;
}