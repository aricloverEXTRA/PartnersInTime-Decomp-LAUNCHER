#ifndef NITRO_CTRDG_H
#define NITRO_CTRDG_H

/*
 * The GBA slot: detecting a cartridge, locking the bus while it is accessed and
 * handling its removal.
 */

#include <nitro/os_lock.h>

typedef int (*CtrdgCallback)(int event);
typedef struct CtrdgLockState { volatile u32 already_locked; u32 interrupts; } CtrdgLockState;
typedef struct CtrdgBusState { u32 rom_cycle, sram_cycle; } CtrdgBusState;
typedef struct CtrdgWork { volatile u16 ready; u16 lock_id; } CtrdgWork;
typedef struct CtrdgModuleInfo {
    u16 module_id;
    u8 extensions[3];
    u8 is_gba : 1;
    u8 removed : 1;
    u8 reserved : 6;
    u16 maker;
    u32 game_code;
} CtrdgModuleInfo;

extern CtrdgWork data_02065728;
extern u32 data_0206572c;
extern u32 data_02065730;
extern CtrdgCallback data_02065734;
typedef char CtrdgModuleInfoSizeCheck[(sizeof(CtrdgModuleInfo) == 12) ? 1 : -1];

void CTRDGi_SendPxi(u32 data);
void CTRDGi_Unlock(u16 lock_id, CtrdgLockState *state);
void CTRDGi_Lock(u16 lock_id, CtrdgLockState *state);
void CTRDGi_RestoreBus(const CtrdgBusState *state);
void CTRDGi_SaveBus(CtrdgBusState *state);
int CTRDG_IsExisting(void);
int CTRDGi_TestModule(u32 mask);
int CTRDG_IsOptionCartridge(u32 mask);
void CTRDGi_InitLock(void);
void CTRDG_TerminateForPulledOut(void);
void CTRDG_SetPulledOutCallback(CtrdgCallback callback);
void CTRDGi_PulledOutCallback(u32 tag, u32 data, int error);
void CTRDGi_InitCallback(u32 tag, u32 data, int error);
void CTRDGi_InitModuleInfo(void);
void CTRDG_Init(void);

extern void WaitByLoop(u32 cycles);
extern void CpuSet(const void *source, void *destination, u32 mode);
extern void OS_Terminate(void);
extern void func_02038e34(u16 lock_id);
extern void PXI_Init(void);
extern int PXI_IsCallbackReady(u32 tag, int processor);
extern int PXI_SendWordByFifo(u32 tag, u32 data, int error);
extern void PXI_SetFifoRecvCallback(u32 tag, void (*callback)(u32, u32, int));

#endif
