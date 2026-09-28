/*
 * Heap allocator (ARM9 resident, 0x02029808-0x02029DE4).
 *
 * The allocator itself: carving a heap out of its parent, serving an allocation
 * from either end of the region, and freeing by merging with the free
 * neighbours. See include/game/heap.h for the block layout.
 */

#include <game/heap.h>

extern GameHeapRegion data_02060b6c[32];

extern void func_0203b7a0(u32 value, void *destination, u32 size);
/* EUR link-time bounds and stack reservations, exported by linker_aliases.json.
 * They are symbol values, not objects to dereference. */
extern u8 GameHeap_MainArenaLow[], GameHeap_ItcmArenaLow[], GameHeap_DtcmArenaLow[];
extern u8 SDK_AUTOLOAD_DTCM_START[], GameHeap_IrqStackSize[], GameHeap_SystemStackSize[];

void GameHeap_Initialize(void)
{
    int i;
    /* The SDK clear operation materializes its zero on the stack. */
    volatile u32 zero = 0;
    func_0203b7a0(zero, data_02060b6c, sizeof(data_02060b6c));

    /* Main RAM and its high-end sentinel, then ITCM, DTCM and shared RAM.
     * Keep first pointers, search cursors, sizes and heap IDs in native order. */
    data_02060b6c[0].first =
        (GameHeapBlock *)(((u32)GameHeap_MainArenaLow + 0x3ff) & -0x200);
    data_02060b6c[1].first = (GameHeapBlock *)0x023dfff0;
    data_02060b6c[2].first = (GameHeapBlock *)GameHeap_ItcmArenaLow;
    data_02060b6c[3].first = (GameHeapBlock *)GameHeap_DtcmArenaLow;
    data_02060b6c[4].first = (GameHeapBlock *)0x027ff000;
    data_02060b6c[0].cursor = data_02060b6c[0].first;
    data_02060b6c[1].cursor = data_02060b6c[1].first;
    data_02060b6c[2].cursor = data_02060b6c[2].first;
    data_02060b6c[3].cursor = data_02060b6c[3].first;
    data_02060b6c[4].cursor = data_02060b6c[4].first;
    data_02060b6c[0].size = 0x023dffe0 - (u32)data_02060b6c[0].first;
    data_02060b6c[1].size = 0;
    data_02060b6c[2].size = 0x01fffff0 - (u32)GameHeap_ItcmArenaLow;
    data_02060b6c[3].size = (u32)SDK_AUTOLOAD_DTCM_START + 0x3f80 - (u32)GameHeap_IrqStackSize -
        (u32)GameHeap_SystemStackSize - (u32)GameHeap_DtcmArenaLow - sizeof(GameHeapBlock);
    data_02060b6c[4].size = 0xbf0;
    data_02060b6c[0].heap = 0;
    data_02060b6c[1].heap = 1;
    data_02060b6c[2].heap = 2;
    data_02060b6c[3].heap = 3;
    data_02060b6c[4].heap = 4;

    /* Heap 1 is the occupied end sentinel for heap 0. The other initial
     * blocks are free; retain the reserved bits in their existing headers. */
    for (i = 0; i < 5; ++i) {
        GameHeapBlock *block = data_02060b6c[i].first;
        switch (i) {
        case 0:
            block->previous = 0;
            block->next = data_02060b6c[1].first;
            break;
        case 1:
            block->previous = data_02060b6c[0].first;
            block->next = 0;
            break;
        case 2:
        case 3:
        case 4:
            block->next = 0;
            block->previous = block->next;
            break;
        }
        block->size_flags = data_02060b6c[i].size;
        block->heap = i;
        if (i != 1) block->size_flags |= 1;
    }
}

void *GameHeap_New(u32 size, int heap, void *unused, int mode)
{
    return GameHeap_Allocate(heap, size, unused, mode);
}

void *GameHeap_NewArray(u32 size, int heap, void *unused, int mode)
{
    return GameHeap_Allocate(heap, size, unused, mode);
}

void GameHeap_Delete(void *allocation)
{
    GameHeap_Free(allocation);
}

void GameHeap_DeleteArray(void *allocation)
{
    GameHeap_Free(allocation);
}

void GameHeap_Merge(GameHeapBlock *first, GameHeapBlock *second)
{
    first->size_flags += (second->size_flags & ~1) + sizeof(GameHeapBlock);
    first->next = second->next;
    if (first->next) first->next->previous = first;
}

void GameHeap_Free(void *allocation)
{
    GameHeapBlock *block;
    GameHeapBlock *previous;
    GameHeapBlock *next;
    u32 heap;
    if (!allocation) return;
    block = (GameHeapBlock *)allocation - 1;
    heap = block->heap;
    next = block->next;
    previous = block->previous;
    block->size_flags |= 1;
    if (next && (next->size_flags & 1)) {
        GameHeap_Merge(block, next);
        if (heap != 1 && data_02060b6c[heap].cursor == next) data_02060b6c[heap].cursor = block;
    }
    if (previous && (previous->size_flags & 1)) {
        GameHeap_Merge(previous, block);
        if (heap != 1) {
            if (data_02060b6c[heap].cursor == block) data_02060b6c[heap].cursor = previous;
        } else {
            if (data_02060b6c[heap].cursor == block) data_02060b6c[heap].cursor = block->next;
        }
    }
}

void *GameHeap_Allocate(int heap, u32 size, void *unused, int mode)
{
    GameHeapBlock *block;
    if (!size) goto failed;
    block = mode == 1 ? data_02060b6c[heap].cursor : data_02060b6c[heap].first;
    size = (size + 3) & ~3;
    if (heap == 1) block = block->previous;
    do {
        u32 size_flags = block->size_flags;
        if ((size_flags & 1) && size_flags >= size) {
            u32 available = size_flags & ~1;
            if (available > size + sizeof(GameHeapBlock)) {
                if (heap != 1) {
                    GameHeapBlock *remainder = (GameHeapBlock *)((u8 *)(block + 1) + size);
                    available -= size + sizeof(GameHeapBlock);
                    remainder->size_flags = available | 1;
                    remainder->previous = block;
                    remainder->next = block->next;
                    if (remainder->next) remainder->next->previous = remainder;
                    block->size_flags = size;
                    block->next = remainder;
                    if ((u32)data_02060b6c[heap].cursor < (u32)remainder) data_02060b6c[heap].cursor = remainder;
                } else {
                    GameHeapBlock *remainder = block;
                    block = (GameHeapBlock *)((u8 *)block + (available - size));
                    block->size_flags = size;
                    block->previous = remainder;
                    block->next = remainder->next;
                    if (block->next) block->next->previous = block;
                    size += sizeof(GameHeapBlock);
                    size = available - size;
                    remainder->size_flags = size | 1;
                    remainder->next = block;
                    if ((u32)data_02060b6c[heap].cursor > (u32)block) data_02060b6c[heap].cursor = block;
                }
            } else {
                block->size_flags = available;
            }
            block->heap = heap;
            return block + 1;
        }
        if (heap != 1) block = block->next;
        else block = block->previous;
    } while (block);
failed:
    return 0;
}

int GameHeap_Create(int parent, u32 size, u32 unused, int mode)
{
    void *allocation = GameHeap_Allocate(parent, size + sizeof(GameHeapBlock), 0, mode);
    int heap;
    if (!allocation) return -1;
    for (heap = 5; heap < 32; ++heap) {
        if (!data_02060b6c[heap].first) {
            GameHeapBlock *block;
            data_02060b6c[heap].first = allocation;
            data_02060b6c[heap].cursor = allocation;
            data_02060b6c[heap].size = size;
            data_02060b6c[heap].heap = heap;
            block = data_02060b6c[heap].first;
            block->next = 0;
            block->previous = block->next;
            block->size_flags = data_02060b6c[heap].size | 1;
            block->heap = heap;
            return heap;
        }
    }
    GameHeap_Free(allocation);
    return -1;
}

void GameHeap_Destroy(int heap)
{
    GameHeap_Free(data_02060b6c[heap].first);
    data_02060b6c[heap].first = 0;
    data_02060b6c[heap].cursor = 0;
    data_02060b6c[heap].size = 0;
}
