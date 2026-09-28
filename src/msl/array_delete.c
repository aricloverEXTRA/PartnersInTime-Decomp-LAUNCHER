/* Runtime array deletion (ARM9, 0x02048874..0x020488BC). */
#include <game/heap.h>
#include <msl/array.h>

void func_02048934(void *array, u32 count, u32 size, MslArrayDestructor destroy);

void MSL_DeleteArray(void *array, u32 size, u32 header, MslArrayDestructor destroy)
{
    if (!array) return;
    /* The shared helper destroys elements from last to first. */
    if (destroy) func_02048934(array, ((u32 *)array)[-1], size, destroy);
    GameHeap_DeleteArray((u8 *)array - header);
}
