/*
 * Core halt (ARM9 resident, 0x0203AE60-0x0203AE6C).
 *
 * Stops the core until an interrupt wakes it, so it returns only once the
 * scheduler has put this thread back on a CPU.
 */

#include <nitro.h>

void OS_Halt(void)
{
    u32 zero = 0;
    /* CP15 maintenance opc7/CRn7/CRm0/opc1=4: halt the core. */
    asm { mcr p15, 0, zero, c7, c0, 4 }
}