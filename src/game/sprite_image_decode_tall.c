/*
 * Tall sprite image decoding: the 8x20 format (ARM9 resident, 0x020233E4-0x020235FC).
 *
 * The decoded image uses the same fixed 20-byte row pitch as the narrow decoders,
 * but each input word pair supplies a low and a high plane for four more rows, so
 * 20 rows are produced from ten source words in two passes of eight and twelve.
 *
 * The second pass declares its planes in the order plane4, plane5, plane0..plane3
 * while still reading them in ascending source order. That declaration order is
 * what drives the compiler's register choice for the six loop-carried planes; the
 * naive ascending order produces a different allocation and 26 differing
 * instructions. See GameSpriteImage_Decode8x16 for the same two-pass shape.
 *
 * Disjoint from GameSpriteImage_Decode8x24 (0x0202314C-0x020233E4), which is not
 * reconstructed yet, so this stays its own unit until that gap closes.
 */
#include <game/sprite_image.h>

void GameSpriteImage_Decode8x20(const u32 *source, u8 *destination)
{
    u32 plane0 = source[0];
    u32 plane1 = source[1];
    u32 plane2 = source[2];
    u32 plane3 = source[3];
    u8 *start = destination;
    s16 remaining = 8;
    do {
        --remaining;
        destination[0] = (plane0 & 1) + 2 * (plane1 & 1);
        destination[20] = ((plane0 & 2) + 2 * (plane1 & 2)) >> 1;
        destination[40] = ((plane0 & 4) + 2 * (plane1 & 4)) >> 2;
        destination[60] = ((plane0 & 8) + 2 * (plane1 & 8)) >> 3;
        destination[80] = (plane2 & 1) + 2 * (plane3 & 1);
        destination[100] = ((plane2 & 2) + 2 * (plane3 & 2)) >> 1;
        destination[120] = ((plane2 & 4) + 2 * (plane3 & 4)) >> 2;
        destination[140] = ((plane2 & 8) + 2 * (plane3 & 8)) >> 3;
        plane0 >>= 4;
        plane1 >>= 4;
        plane2 >>= 4;
        plane3 >>= 4;
        ++destination;
    } while (remaining > 0);
    {
        u32 plane4;
        u32 plane5;
        u32 plane0;
        u32 plane1;
        u32 plane2;
        u32 plane3;
        plane0 = source[4];
        plane1 = source[5];
        plane2 = source[6];
        plane3 = source[7];
        plane4 = source[8];
        plane5 = source[9];
        destination = start;
        remaining = 8;
        do {
            --remaining;
            destination[160] = (plane0 & 1) + 2 * (plane1 & 1);
            destination[180] = ((plane0 & 2) + 2 * (plane1 & 2)) >> 1;
            destination[200] = ((plane0 & 4) + 2 * (plane1 & 4)) >> 2;
            destination[220] = ((plane0 & 8) + 2 * (plane1 & 8)) >> 3;
            destination[240] = (plane2 & 1) + 2 * (plane3 & 1);
            destination[260] = ((plane2 & 2) + 2 * (plane3 & 2)) >> 1;
            destination[280] = ((plane2 & 4) + 2 * (plane3 & 4)) >> 2;
            destination[300] = ((plane2 & 8) + 2 * (plane3 & 8)) >> 3;
            destination[320] = (plane4 & 1) + 2 * (plane5 & 1);
            destination[340] = ((plane4 & 2) + 2 * (plane5 & 2)) >> 1;
            destination[360] = ((plane4 & 4) + 2 * (plane5 & 4)) >> 2;
            destination[380] = ((plane4 & 8) + 2 * (plane5 & 8)) >> 3;
            plane0 >>= 4;
            plane1 >>= 4;
            plane2 >>= 4;
            plane3 >>= 4;
            plane4 >>= 4;
            plane5 >>= 4;
            ++destination;
        } while (remaining > 0);
    }
}