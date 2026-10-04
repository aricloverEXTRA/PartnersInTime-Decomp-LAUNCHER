/*
 * Zero query (ARM9 resident, 0x02039BAC-0x02039BB4).
 *
 * Takes no arguments and returns 0. It sits in the OS thread and lock region
 * but is not yet attributed to a specific interface.
 */

#include <nitro.h>

u32 func_02039bac(void)
{
    return 0;
}