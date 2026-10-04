/*
 * Zero query (ARM9 resident, 0x02046FF8-0x02047000).
 *
 * Takes no arguments and returns 0. It lies between the MSL unwind teardown and
 * MSL_WriteConsole but is not yet attributed to a specific interface.
 */

#include <nitro.h>

u32 func_02046ff8(void)
{
    return 0;
}