/* ARM9 exception callback dispatch, 0x0203A600..0x0203A66C. */
#include <nitro.h>

typedef void (*OSExceptionCallback)(void *context, void *argument);

extern OSExceptionCallback data_02062f60;
extern void *data_02062f64;
extern u8 data_02062f6c[];

void OS_EnableProtectionUnit(void);
void OS_DisableProtectionUnit(void);

void OSi_CallUserExceptionHandler(void)
{
    if (!data_02062f60) return;

    /* Preserve the exception stack across the banked-SP mode switch.
     * 0x9F selects System mode with IRQs masked and condition flags cleared.
     * The exception path deliberately leaves this mode and the MPU disabled
     * when the user callback returns; it does not restore the caller's state.
     */
    asm {
        mov r0, sp
        ldr r1, =0x9f
        msr CPSR_cxsf, r1
        mov sp, r0
    }
    OS_EnableProtectionUnit();
    data_02062f60(data_02062f6c, data_02062f64);
    OS_DisableProtectionUnit();
}
