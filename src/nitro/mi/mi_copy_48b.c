/*
 * Fixed-size 48-byte block copy (ARM9 resident, 0x0203B888-0x0203B8AC).
 *
 * MTX44 quaternions and matrix rows are exactly three words wide, so the
 * length is known at compile time and the copy unrolls into four
 * load/store-multiple pairs with no register save required.
 */

#include <nitro.h>

void MI_Copy48B(const void *source, void *destination)
{
    asm {
        ldmia r0!, {r2, r3, r12}
        stmia r1!, {r2, r3, r12}
        ldmia r0!, {r2, r3, r12}
        stmia r1!, {r2, r3, r12}
        ldmia r0!, {r2, r3, r12}
        stmia r1!, {r2, r3, r12}
        ldmia r0!, {r2, r3, r12}
        stmia r1!, {r2, r3, r12}
    }
}