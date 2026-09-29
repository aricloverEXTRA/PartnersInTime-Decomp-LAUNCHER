#ifndef NITRO_GX_COMMAND_LIST_H
#define NITRO_GX_COMMAND_LIST_H

/*
 * Packing 3D commands into the FIFO format the geometry engine consumes.
 */

#include <nitro.h>

typedef struct GxCommandList {
    u8 *command;
    u32 *parameters;
    u8 *buffer;
    u32 capacity;
    u32 padding_required;
} GxCommandList;

/* These routines only manage packing cursors. Capacity is recorded here;
 * callers remain responsible for allocating and bounding the backing buffer. */
#ifdef __cplusplus
extern "C" {
#endif
void GxCommandList_Begin(GxCommandList *list, void *buffer, u32 capacity);
u32 GxCommandList_End(GxCommandList *list);
#ifdef __cplusplus
}
#endif

typedef char GxCommandList_SizeCheck[sizeof(GxCommandList) == 20 ? 1 : -1];

#endif
