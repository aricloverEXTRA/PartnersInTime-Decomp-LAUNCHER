#ifndef NITRO_MI_DMA_H
#define NITRO_MI_DMA_H

#include <nitro.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Raw variants leave IRQ protection to their caller. The reset variants
 * perform the native DMA0 read delays and replace channel 0 with a dummy
 * transfer after programming it. Other channels retain their parameters. */
void MIi_DmaSetParamsRaw(u32 channel, const void *source, void *destination, u32 control);
void MIi_DmaSetParamsRawAndReset0(u32 channel, const void *source, void *destination, u32 control);
void MIi_DmaSetParams(u32 channel, const void *source, void *destination, u32 control);
void MIi_DmaSetParamsAndReset0(u32 channel, const void *source, void *destination, u32 control);

/* Submit at most 472 bytes per DMA request to the geometry command FIFO. */
void MI_SendGXCommand(u32 channel, const void *source, u32 size);

#ifdef __cplusplus
}
#endif

#endif
