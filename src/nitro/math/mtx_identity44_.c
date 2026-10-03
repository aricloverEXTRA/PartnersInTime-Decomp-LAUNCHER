/*
 * 4x4 fixed-point identity matrix (ARM9 resident, 0x020341E8-0x02034214).
 *
 * Writes the 44-byte identity matrix using the SDK's exact STM sequence
 * with Q12 constants (0x1000 = 1.0, 0x0000 = 0.0). MWCC cannot reproduce
 * the exact STM register-list order and immediate values from C, so the
 * body is a single explicit assembly fragment.
 */

#include <nitro/fx_mtx.h>

void MTX_Identity44_(MtxFx44 *m)
{
    asm {
        mov r2, #0x1000
        mov r3, #0
        stmia r0!, {r2, r3}
        mov r1, #0
        stmia r0!, {r1, r3}
        stmia r0!, {r1, r2, r3}
        stmia r0!, {r1, r3}
        stmia r0!, {r1, r2, r3}
        stmia r0!, {r1, r3}
        stmia r0!, {r1, r2}
    }
}