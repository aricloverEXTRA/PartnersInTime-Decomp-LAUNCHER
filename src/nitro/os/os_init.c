/*
 * OS initialization (ARM9 resident, 0x0203A068-0x0203A0B4).
 *
 * Brings the OS subsystems up in the order they depend on each other.
 */

#include <nitro/os_lock.h>
#include <nitro/os_arena.h>
#include <nitro/os_alarm.h>
#include <nitro/os_reset.h>
#include <nitro/pxi.h>

extern void func_02038e20(void);
extern void OS_InitException(void);
extern void func_0203bd48(void);
extern void OSi_InitVramExclusive(void);
extern void func_020396d8(void);
extern void CTRDG_Init(void);
extern void CARD_Init(void);
extern void PM_Init(void);

void OS_Init(void)
{
    OS_InitArena();
    PXI_Init();
    OS_InitLock();
    OS_InitArenaEx();
    func_02038e20();
    OS_InitException();
    func_0203bd48();
    OS_InitVAlarm();
    OSi_InitVramExclusive();
    func_020396d8();
    OS_InitReset();
    CTRDG_Init();
    CARD_Init();
    PM_Init();
}
