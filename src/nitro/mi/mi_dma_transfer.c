/*
 * DMA transfers (ARM9 resident, 0x0203B500-0x0203B76C).
 *
 * The copy and fill transfers the rest of the game requests DMA through.
 */

#include <nitro/mi_dma.h>
#include <nitro/os_sync.h>

void MIi_CheckDma0SourceAddress(u32 channel, const void *source, u32 size, u32 mode);
void MIi_CheckAnotherAutoDMA(u32 channel, u32 timing);
void func_0203b310(u32 channel);

#define DMA_WORDS ((vu32 *)0x040000B0)
#define DMA_FILL_DATA ((vu32 *)0x040000E0)

void func_0203b6f8(u32 channel, const void *source, void *destination, u32 size) {
    MIi_CheckAnotherAutoDMA(channel, 0x10000000);
    MIi_CheckDma0SourceAddress(channel, source, size, 0);
    if (!size) return;
    func_0203b310(channel);
    MIi_DmaSetParams(channel, source, destination, 0x96600000 | (size >> 2));
}

void func_0203b684(u32 channel, const void *source, void *destination, u32 size) {
    MIi_CheckAnotherAutoDMA(channel, 0x10000000);
    MIi_CheckDma0SourceAddress(channel, source, size, 0);
    if (!size) return;
    func_0203b310(channel);
    MIi_DmaSetParams(channel, source, destination, 0x92600000 | (size >> 1));
}

void func_0203b5f4(u32 channel, void *destination, u32 value, u32 size) {
    vu32 *control;
    u32 state;
    if (!size) return;
    control = &DMA_WORDS[3 * channel + 2];
    while (*control & 0x80000000) {}
    state = OS_DisableInterrupts();
    DMA_FILL_DATA[channel] = value;
    MIi_DmaSetParamsRawAndReset0(channel, (const void *)((u32)DMA_FILL_DATA + 4 * channel), destination,
                   0x85000000 | (size >> 2));
    OS_RestoreInterrupts(state);
    while (*control & 0x80000000) {}
}

void func_0203b578(u32 channel, const void *source, void *destination, u32 size) {
    vu32 *control;
    MIi_CheckDma0SourceAddress(channel, source, size, 0);
    if (!size) return;
    control = &DMA_WORDS[3 * channel + 2];
    while (*control & 0x80000000) {}
    MIi_DmaSetParamsAndReset0(channel, source, destination, 0x84000000 | (size >> 2));
    while (*control & 0x80000000) {}
}

void func_0203b500(u32 channel, const void *source, void *destination, u32 size) {
    vu32 *control;
    if (!size) return;
    MIi_CheckDma0SourceAddress(channel, source, size, 0);
    control = &DMA_WORDS[3 * channel + 2];
    while (*control & 0x80000000) {}
    MIi_DmaSetParamsAndReset0(channel, source, destination, 0x80000000 | (size >> 1));
    while (*control & 0x80000000) {}
}
