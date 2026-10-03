/*
 * Global interrupt disable (ARM9 resident, 0x0203ADFC-0x0203AE10).
 *
 * Raises the CPSR I bit and returns its previous value as a bit mask, so the
 * caller can pass the result straight back to OS_RestoreInterrupts without
 * shifting it.
 *
 * MWCC rejects a C variable as the operand of "msr CPSR_c"; the entire round
 * trip is one explicit-register assembly fragment. The result is captured in
 * a C variable so MWCC knows r0 holds the final value.
 */

#include <nitro.h>

u32 OS_DisableInterrupts(void)
{
    u32 result;

    asm {
        mrs result, CPSR
        orr r1, result, #0x80
        msr CPSR_c, r1
        and result, result, #0x80
    }
    return result;
}