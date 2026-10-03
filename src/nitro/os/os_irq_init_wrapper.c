/*
 * IRQ setup wrapper (ARM9 resident, unlinked draft).
 *
 * Unlinked draft. Gap documented in source.
 */

#include <nitro.h>

﻿/*
 * IRQ setup wrapper (ARM9 resident, unlinked draft).
 *
 * Calls the two IRQ initialization helpers in sequence. The push/pop
 * is explicit asm so MWCC emits the exact stmdb/ldm pair.
 *
 * The C calls to func_0203a570/func_0203a600 go through veneers rather than
 * direct relative branches, adding 16 bytes. Native uses direct bl to fixed
 * addresses. Revisit when the branch mechanism is understood.
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