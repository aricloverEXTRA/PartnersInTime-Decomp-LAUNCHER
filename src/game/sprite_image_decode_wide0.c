/*
 * Wide sprite image decode: the 0x02023000 variant (ARM9 resident, 332 bytes).
 *
 * This format decodes through the 8x12 helper first, then repacks three further
 * source words across the same fixed 20-byte row pitch. It is a disjoint
 * .text range and stays in its own unit while the gap before
 * 0x020235FC remains.
 */
#include <game/sprite_image.h>

void func_02023000(const u32 *source, u8 *destination)
{
    u32 low0;
    u32 high0;
    u32 low1;
    u32 high1;
    u32 low2;
    u32 high2;
    s16 remaining;

    GameSpriteImage_Decode8x12(source, destination);
    destination += 8;
    low0 = source[6];
    high0 = low0 >> 15;
    low1 = source[7];
    high1 = low1 >> 15;
    low2 = source[8];
    high2 = low2 >> 15;
    remaining = 4;
    do {
        --remaining;
        destination[0] = (low0 & 1) + (high0 & 2);
        destination[20] = ((low0 & 2) + (high0 & 4)) >> 1;
        destination[40] = ((low0 & 4) + (high0 & 8)) >> 2;
        destination[60] = ((low0 & 8) + (high0 & 0x10)) >> 3;
        destination[80] = (low1 & 1) + (high1 & 2);
        destination[100] = ((low1 & 2) + (high1 & 4)) >> 1;
        destination[120] = ((low1 & 4) + (high1 & 8)) >> 2;
        destination[140] = ((low1 & 8) + (high1 & 0x10)) >> 3;
        destination[160] = (low2 & 1) + (high2 & 2);
        destination[180] = ((low2 & 2) + (high2 & 4)) >> 1;
        destination[200] = ((low2 & 4) + (high2 & 8)) >> 2;
        destination[220] = ((low2 & 8) + (high2 & 0x10)) >> 3;
        low0 >>= 4;
        high0 >>= 4;
        low1 >>= 4;
        high1 >>= 4;
        low2 >>= 4;
        high2 >>= 4;
        ++destination;
    } while (remaining > 0);
}