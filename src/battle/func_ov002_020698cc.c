/*
 * Battle AI party script query (ov002, 0x020698CC-0x020698D4).
 *
 * The VM and state arguments are accepted but unused; native returns the
 * immediate 2 in r0 and the caller discards it. The existing declaration in
 * battle_ai_runtime.c claimed void, which the callee's return path contradicts.
 */

#include <nitro.h>
#include <game/battle_ai.h>

u32 func_ov002_020698cc(ScriptVm *vm, ScriptVmState *state)
{
    return 2;
}