/*
 * Processor mode query (ARM9 resident, 0x0203AE54-0x0203AE60).
 *
 * The scheduler reads this to tell whether the current context is still in
 * system mode, which decides if it is safe to touch shared state.
 */

#include <nitro.h>

int OS_GetProcMode(void)
{
    int mode;
    /* CPSR in user mode reads back as APSR, whose low five bits are the mode. */
    asm { mrs mode, cpsr }
    return mode & 0x1f;
}