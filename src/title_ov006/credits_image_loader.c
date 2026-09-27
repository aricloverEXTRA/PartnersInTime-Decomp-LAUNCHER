/*
 * Credits illustration loading (overlay 6, 0x020786E0-0x02078938).
 *
 * Reads the next illustration out of the archive while the current one is still
 * on screen, so the transition never waits on the filesystem.
 */

#include "credits_transition_internal.h"

/* Fill the inactive texture slot in 16 chunks before switching illustrations. */
void CreditsImage_LoadNext(MenuElement *element)
{
    int image = ((const s8 *)&data_ov006_0207bfe8)[16 * CREDITS_TRANSITION.layout];
    switch (element->state) {
    case 0:
        func_ov005_02066ed4(ARCHIVE, 1, data_ov006_0207bed8[image].entry, &element->resource, 0);
        ++element->state;
        break;
    case 1:
        if (func_ov005_02067238(ARCHIVE, 1, data_ov006_0207bed8[image].entry)) {
            MI_UncompressLZ16(element->resource, data_ov006_0207c5cc);
            if (element->resource) {
                GameHeap_DeleteArray(element->resource);
                element->resource = 0;
            }
            element->arguments[0] = 0;
            ++element->state;
        }
        break;
    case 2:
        func_ov005_0206659c(CreditsImage_UploadTextureChunk, 11, 1)->arguments[0] = element->arguments[0];
        ++element->arguments[0];
        if (element->arguments[0] >= 16)
            ++element->state;
        break;
    case 3:
        func_ov005_02066ed4(ARCHIVE, 1, data_ov006_0207beda[image].entry, &element->resource, 0);
        ++element->state;
        break;
    case 4:
        if (func_ov005_02067238(ARCHIVE, 1, data_ov006_0207beda[image].entry)) {
            GameResource_Move16(element->resource, (s16 *)&data_ov006_020845cc, 512);
            if (element->resource) {
                GameHeap_DeleteArray(element->resource);
                element->resource = 0;
            }
            ++element->state;
        }
        break;
    case 5:
        func_ov005_0206659c(func_ov006_02078938, 11, 1);
        ++element->state;
        break;
    case 6:
        CREDITS_TRANSITION.image_loading = 0;
        func_ov005_0206650c(element);
        break;
    }
}
