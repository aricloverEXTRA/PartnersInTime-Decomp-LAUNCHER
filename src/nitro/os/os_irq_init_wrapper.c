/*
 * IRQ setup wrapper (ARM9 resident, unlinked draft).
 *
 * Unlinked draft. Gap documented in source.
 */

#include <nitro.h>

extern void func_0203a570(void);
extern void func_0203a600(void);

void func_0203a55c(void)
{
    asm { stmdb sp!, {lr} }
    func_0203a570();
    func_0203a600();
    asm { ldmia sp!, {lr} }
}