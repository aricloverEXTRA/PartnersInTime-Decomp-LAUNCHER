/* Credits illustration texture upload, overlay 6, 0x02078990-0x020789F4.
 * The loader schedules all sixteen 2 KiB chunks into the inactive 32 KiB slot.
 */
#include "credits_transition_internal.h"

void CreditsImage_UploadTextureChunk(MenuElement *element)
{
    int chunk = element->arguments[0];
    int offset = (chunk << 11) + ((1 - CREDITS_TRANSITION.variant) << 15);
    u8 *source = data_ov006_0207c5cc + 2048 * chunk;
    // Flush the staging bytes before mapping the texture banks for DMA.
    DC_FlushRange(source, 2048);
    func_02038984();
    GX_LoadTex(source, offset, 2048);
    // Wait for the transfer, restore the bank mapping, then retire this job.
    func_020387b0();
    func_ov005_0206650c(element);
}
