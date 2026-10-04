/*
 * Zero query (ARM9 resident, 0x0203D730-0x0203D738).
 *
 * Takes no arguments and returns 0. It precedes the FSi_OpenFile command
 * wrappers but is not yet attributed to a specific interface.
 */

#include <nitro.h>

u32 func_0203d730(void)
{
    return 0;
}