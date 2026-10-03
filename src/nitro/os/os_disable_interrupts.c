/*
 * Global interrupt disable (ARM9 resident, 0x0203ADFC-0x0203AE10).
 *
 * Raises the CPSR I bit and returns its previous value as a bit mask, so the
 * caller can pass the result straight back to OS_RestoreInterrupts without
 * shifting it.
 *
 * The CPSR round trip is kept in one assembly fragment because splitting it
 * across statements changes which registers MWCC allocates. Only the read and
 * the final mask touch C variables; r1 is a scratch register for the value
 * handed to CPSR.
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