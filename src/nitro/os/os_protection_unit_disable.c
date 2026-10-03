/*
 * MPU disable (ARM9 resident, 0x0203A4C0-0x0203A4D0).
 *
 * Clears the protection-unit bit again, which lets the game touch regions
 * that are still described but no longer guarded.
 */

#include <nitro.h>

void OS_DisableProtectionUnit(void)
{
    u32 control;
    asm {
        mrc p15, 0, control, c1, c0, 0
        bic control, control, #1
        mcr p15, 0, control, c1, c0, 0
    }
}
