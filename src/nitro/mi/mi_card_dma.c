/*
 * Card DMA copy (ARM9 resident, 0x0203BCBC-0x0203BD48).
 *
 * The 32-bit DMA copy the card subsystem transfers with.
 */

#include <nitro/mi_dma.h>
#include <nitro/os_sync.h>
extern void MIi_CheckAnotherAutoDMA(u32, u32);
extern void MIi_CheckDma0SourceAddress(u32, const void *, u32, u32);
void MIi_CardDmaCopy32(u32 channel, const void *source, void *destination, u32 size) {
    vu32 *control;
    MIi_CheckAnotherAutoDMA(channel, -1);
    MIi_CheckDma0SourceAddress(channel, source, size, 0x1000000);
    if (!size)
        return;
    control = &((vu32 *)0x040000b0)[3 * channel + 2];
    while (*control & 0x80000000) {
    }
    MIi_DmaSetParams(channel, source, destination, 0xaf000001);
}
