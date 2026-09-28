#ifndef PIT_GAME_HEAP_H
#define PIT_GAME_HEAP_H

#include <nitro.h>

/* The game's allocator. Up to 32 numbered heaps are carved out of each other:
   creating a heap allocates a block inside a parent heap and turns it into a
   region of its own, so a scene can drop everything it allocated by destroying
   its heap instead of freeing block by block.

   Each region is a doubly linked list of blocks in address order. Every
   allocation is preceded by its own header, so GameHeap_Free recovers the
   header by stepping one GameHeapBlock back from the pointer it is given. */

typedef struct GameHeapBlock GameHeapBlock;

/* The 16-byte header in front of every allocation. */
struct GameHeapBlock {
    GameHeapBlock *previous;
    GameHeapBlock *next;
    /* Payload size with the free flag in bit 0: set means this block is
       available and may be merged with a free neighbour. */
    u32 size_flags;
    /* Which heap the block belongs to, so freeing needs only the pointer. */
    u32 heap : 5;
    u32 reserved : 27;
};

/* One numbered heap. `cursor` is where the next search starts, so allocation
   rotates through the region instead of always scanning from `first`. */
typedef struct GameHeapRegion {
    GameHeapBlock *first;
    GameHeapBlock *cursor;
    u32 size;
    u32 heap : 5;
    u32 reserved : 27;
} GameHeapRegion;

typedef char GameHeapBlockSizeCheck[(sizeof(GameHeapBlock) == 16) ? 1 : -1];
typedef char GameHeapRegionSizeCheck[(sizeof(GameHeapRegion) == 16) ? 1 : -1];

/* `mode` selects which end of the region a request is served from, so
   long-lived and short-lived allocations can be kept apart. `unused` is a
   parameter the original API carries but never reads. */
/* Establish the five initial regions during boot, before allocations. */
void GameHeap_Initialize(void);
void GameHeap_Destroy(int heap);
int GameHeap_Create(int parent, u32 size, u32 unused, int mode);
void *GameHeap_Allocate(int heap, u32 size, void *unused, int mode);
void GameHeap_Free(void *allocation);
/* Absorb `second` into `first`; both must be adjacent blocks of one region. */
void GameHeap_Merge(GameHeapBlock *first, GameHeapBlock *second);
/* operator new/new[]/delete/delete[] as the original build emitted them: thin
   wrappers over Allocate and Free with the size argument moved to the front. */
void GameHeap_DeleteArray(void *allocation);
void GameHeap_Delete(void *allocation);
void *GameHeap_NewArray(u32 size, int heap, void *unused, int mode);
void *GameHeap_New(u32 size, int heap, void *unused, int mode);

#endif
