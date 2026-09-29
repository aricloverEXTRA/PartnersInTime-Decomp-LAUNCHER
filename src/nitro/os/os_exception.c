/* ARM9 exception callbacks and vector initialization, 0x0203A600..0x0203A6FC. */
#include <nitro.h>

typedef void (*OSExceptionCallback)(void *context, void *argument);

extern OSExceptionCallback data_02062f60;
extern void *data_02062f64;
extern u8 data_02062f6c[];

void OS_EnableProtectionUnit(void);
void OS_DisableProtectionUnit(void);

extern void (*data_02062f68)(void);
extern u32 func_02039b34(void);
extern void func_0203a4e8(void);

void OS_InitException(void)
{
    u32 debugger = *(u32 *)0x027FFD9C;

    /* Preserve only a debugger vector inside the expected RAM window. */
    if (debugger >= 0x02600000 && debugger < 0x02800000)
        data_02062f68 = (void (*)(void))debugger;
    else
        data_02062f68 = 0;

    if (!data_02062f68 || !(func_02039b34() & 0x40000000)) {
        /* Keep the DTCM vector's native page base plus offset. MWCC folds
         * the equivalent C store into the absolute address 0x027E3FDC.
         * Both the shared-RAM slot and DTCM slot receive the native entry. */
        asm {
            ldr r2, =func_0203a4e8
            ldr r1, =0x027ffd9c
            ldr r0, =0x027e3000
            str r2, [r1]
            str r2, [r0, #0xfdc]
        }
    }
    data_02062f60 = 0;
}

void OSi_CallUserExceptionHandler(void)
{
    if (!data_02062f60) return;

    /* Preserve the exception stack across the banked-SP mode switch.
     * 0x9F selects System mode with IRQs masked and condition flags cleared.
     * After the callback, this path stays in System mode and disables the MPU.
     * The caller's previous CPU mode and MPU state are not restored.
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
