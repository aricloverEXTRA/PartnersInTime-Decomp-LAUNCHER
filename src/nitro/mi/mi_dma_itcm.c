/* ITCM DMA register programming and geometry FIFO submission.
 * Native range 0x01FF84C0..0x01FF86C8; the raw variants rely on caller IRQ state.
 */
#include <nitro/os_sync.h>
#include <nitro/mi_dma.h>

#define DMA_WORDS ((vu32 *)0x040000b0)

extern void MIi_CheckDma0SourceAddress(u32 channel, const void *source, u32 size, u32 mode);

void MI_SendGXCommand(u32 channel, const void *source, u32 size)
{
    vu32 *control;
    if (!size) return;
    MIi_CheckDma0SourceAddress(channel, source, size, 0);
    control = &DMA_WORDS[3 * channel + 2];
    while (*control & 0x80000000) {}
    while (size) {
        u32 chunk = size > 0x1d8 ? 0x1d8 : size;
        MIi_DmaSetParams(channel, source, (void *)0x04000400, 0x84400000 | (chunk >> 2));
        size -= chunk;
        source = (const u8 *)source + chunk;
    }
    while (*control & 0x80000000) {}
}

void MIi_DmaSetParams(u32 channel, const void *source, void *destination, u32 control)
{
    u32 state = OS_DisableInterrupts();
    vu32 *registers = (vu32 *)((u8 *)DMA_WORDS + 12 * channel);
    registers[0] = (u32)source;
    registers[1] = (u32)destination;
    registers[2] = control;
    OS_RestoreInterrupts(state);
}

void MIi_DmaSetParamsAndReset0(u32 channel, const void *source, void *destination, u32 control)
{
    u32 state = OS_DisableInterrupts();
    vu32 *registers = (vu32 *)((u8 *)DMA_WORDS + 12 * channel);
    registers[0] = (u32)source;
    registers[1] = (u32)destination;
    registers[2] = control;
    /* These reads are from DMA0 even when programming another channel. */
    (void)*DMA_WORDS;
    (void)*DMA_WORDS;
    if (!channel) {
        registers[0] = 0;
        registers[1] = 0;
        registers[2] = 0x81400001;
    }
    OS_RestoreInterrupts(state);
}

void MIi_DmaSetParamsRaw(u32 channel, const void *source, void *destination, u32 control)
{
    vu32 *registers = (vu32 *)((u8 *)DMA_WORDS + 12 * channel);
    registers[0] = (u32)source;
    registers[1] = (u32)destination;
    registers[2] = control;
}

void MIi_DmaSetParamsRawAndReset0(u32 channel, const void *source, void *destination, u32 control)
{
    vu32 *registers = (vu32 *)((u8 *)DMA_WORDS + 12 * channel);
    registers[0] = (u32)source;
    registers[1] = (u32)destination;
    registers[2] = control;
    (void)*DMA_WORDS;
    (void)*DMA_WORDS;
    if (!channel) {
        registers[0] = 0;
        registers[1] = 0;
        registers[2] = 0x81400001;
    }
    (void)*DMA_WORDS;
    (void)*DMA_WORDS;
}
