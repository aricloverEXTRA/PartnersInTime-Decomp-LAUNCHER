/*
 * Unimplemented MSL handlers (ARM9 resident, 0x02046DC0-0x02046DD4).
 *
 * Three entry points with no body work: an empty stub, one returning 0, and
 * one returning the 0x40000000 flag bit. They are declared in reverse address
 * order because this compiler emits functions in reverse source order.
 *
 * The bodies are not yet attributed to specific MSL interfaces.
 */

#include <nitro.h>

u32 func_02046dcc(void)
{
    return 0x40000000;
}

u32 func_02046dc4(void)
{
    return 0;
}

void func_02046dc0(void)
{
}