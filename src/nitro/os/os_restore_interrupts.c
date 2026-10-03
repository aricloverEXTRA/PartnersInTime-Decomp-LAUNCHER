/*
 * Global interrupt restore (ARM9 resident, 0x0203AE10-0x0203AE28).
 *
 * Replaces the CPSR I bit with the saved bit and returns the value that bit
 * held before the change, which lets nested callers detect whether they were
 * the ones that actually disabled interrupts.
 *
 * The CPSR round trip is kept in one assembly fragment because splitting it
 * across statements changes which registers MWCC allocates. r0 still carries
 * the incoming state, which the fragment folds into the value written to CPSR.
 */

#include <nitro.h>

u32 OS_RestoreInterrupts(u32 state)
{
    u32 result;

    asm {
        mrs r1, CPSR
        bic r2, r1, #0x80
        orr r2, r2, r0
        msr CPSR_c, r2
        and result, r1, #0x80
    }
    return result;
}