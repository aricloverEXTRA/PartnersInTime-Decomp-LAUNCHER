/*
 * Soft reset (ARM9 resident, 0x0203AE8C-0x0203AFE8).
 *
 * Quiesces card/DMA transfers, sends the ARM7 reset command and enters the
 * ITCM restart routine. The FIFO callback acknowledges the other processor.
 */

#include <nitro/os_reset.h>
#include <nitro/os_lock.h>
#include <nitro/card.h>
#include <nitro/pxi.h>

extern void CARD_LockRom(u16 lock_id);
extern u32 func_02038cc4(u32 mask);
extern void func_01ff8480(void);
extern u16 data_02063020;
extern u16 data_02063024;
void OSi_ResetCallback(u32 tag, u32 data, int error);

void OS_InitReset(void)
{
    if (data_02063020) return;
    data_02063020 = 1;
    PXI_Init();
    while (!PXI_IsCallbackReady(12, 1)) {}
    PXI_SetFifoRecvCallback(12, OSi_ResetCallback);
}

void OSi_ResetCallback(u32 tag, u32 data, int error)
{
    u16 command = (data & 0x7F00) >> 8;
    if (command == 16) data_02063024 = 1;
    else OS_Terminate();
}

void OSi_SendResetCommand(u32 command)
{
    while (PXI_SendWordByFifo(12, command << 8, 0)) {}
}

void OS_ResetSystem(u32 parameter)
{
    /* The download-boot path cannot restart from the card image. */
    if (*(vu16 *)0x027ffc40 == 2) OS_Terminate();
    CARD_LockRom((u16)OS_GetLockID());
    MI_StopDma(0);
    MI_StopDma(1);
    MI_StopDma(2);
    MI_StopDma(3);
    /* Keep the receive-FIFO IRQ enabled for the ARM7 acknowledgement. */
    func_02038cc4(0x40000);
    OS_ResetRequestIrqMask(~0u);
    *(vu32 *)0x027ffc20 = parameter;
    OSi_SendResetCommand(16);
    func_01ff8480();
}
