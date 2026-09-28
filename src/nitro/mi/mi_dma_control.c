/*
 * DMA control (ARM9 resident, 0x0203B158-0x0203B428).
 *
 * Starting and stopping DMA channels, with the checks that stop a transfer from
 * being programmed over one that is already running.
 */

#include <nitro/mi_dma.h>
#include <nitro/os_sync.h>

typedef void (*DmaCallback)(void *argument);
extern void OS_Terminate(void);
extern void func_02038d44(int channel, DmaCallback callback, void *argument);
void MIi_CheckDma0SourceAddress(u32 channel, const void *source, u32 size, u32 mode);
void MIi_CheckAnotherAutoDMA(u32 channel, u32 timing);
void func_0203b310(u32 channel);

#define DMA_WORDS ((vu32 *)0x040000B0)
#define DMA_HALFWORDS ((vu16 *)0x040000B0)

void func_0203b380(u32 channel, const void *source, void *destination, u32 size,
                   DmaCallback callback, void *argument) {
    MIi_CheckDma0SourceAddress(channel, source, size, 0);
    if (!size) {
        if (callback) callback(argument);
        return;
    }
    func_0203b310(channel);
    if (callback) {
        func_02038d44(channel, callback, argument);
        MIi_DmaSetParams(channel, source, destination, 0xC4000000 | (size >> 2));
    } else {
        MIi_DmaSetParams(channel, source, destination, 0x84000000 | (size >> 2));
    }
}

/* Channel zero needs a harmless transfer after it becomes idle. */
static inline void ResetDma0(u32 channel) {
    vu32 *registers = (vu32 *)((u8 *)DMA_WORDS + 12 * channel);
    registers[0] = 0;
    registers[1] = 0;
    registers[2] = 0x81400001;
}

void func_0203b310(u32 channel) {
    u32 state = OS_DisableInterrupts();
    vu32 *control = &DMA_WORDS[3 * channel + 2];
    while (*control & 0x80000000) {}
    if (!channel) ResetDma0(channel);
    OS_RestoreInterrupts(state);
}

void MI_StopDma(u32 channel) {
    u32 state = OS_DisableInterrupts();
    vu16 *control = &DMA_HALFWORDS[6 * channel + 5];
    *control &= ~0x3A00;
    *control &= ~0x8000;
    (void)*control;
    (void)*control;
    if (!channel) ResetDma0(channel);
    OS_RestoreInterrupts(state);
}

void MIi_CheckAnotherAutoDMA(u32 channel, u32 timing) {
    int index;
    vu32 *control = (vu32 *)0x040000B8;
    for (index = 0; index < 3; index++, control += 3) {
        if (index != channel) {
            u32 value = *control;
            if (value & 0x80000000) {
                u32 other_timing = value & 0x38000000;
                if (other_timing != timing &&
                    !(other_timing == 0x08000000 && timing == 0x10000000) &&
                    !(other_timing == 0x10000000 && timing == 0x08000000) &&
                    (other_timing == 0x18000000 || other_timing == 0x20000000 ||
                     other_timing == 0x28000000 || other_timing == 0x30000000 ||
                     other_timing == 0x38000000 || other_timing == 0x08000000 ||
                     other_timing == 0x10000000)) OS_Terminate();
            }
        }
    }
}

void MIi_CheckDma0SourceAddress(u32 channel, const void *source, u32 size, u32 mode) {
    u32 start;
    if (channel) return;
    start = (u32)source & 0xFF000000;
    if (mode) {
        if (mode == 0x00800000) source = (const u8 *)source - size;
    } else {
        source = (const u8 *)source + size;
    }
    if (start == 0x04000000 || start >= 0x08000000 ||
        ((u32)source & 0xFF000000) == 0x04000000 ||
        ((u32)source & 0xFF000000) >= 0x08000000) OS_Terminate();
}

void func_0203b158(u32 value) { *(vu8 *)0x04000247 = value; }
