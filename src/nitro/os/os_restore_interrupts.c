/*
 * Global interrupt restore (ARM9 resident, 0x0203AE10-0x0203AE28).
 *
 * Replaces the CPSR I bit with the saved bit and returns the value that bit
 * held before the change, which lets nested callers detect whether they were
 * the ones that actually disabled interrupts.
 *
 * MWCC rejects a C variable as the operand of "msr CPSR_c"; the entire round
 * trip is one explicit-register assembly fragment. The result is captured in
 * a C variable so MWCC knows r0 holds the final value.
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