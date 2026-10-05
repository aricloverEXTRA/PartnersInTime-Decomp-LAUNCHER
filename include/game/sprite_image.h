#ifndef PIT_GAME_SPRITE_IMAGE_H
#define PIT_GAME_SPRITE_IMAGE_H

/*
 * The sprite image formats and the decoders that expand them into tile memory.
 */
#include <game/sprite_effect.h>

GameSpriteEffectSize GameSpriteImage_Draw(u16 x, u16 y, s32 scale_x, s32 scale_y,
    u8 image_index, u16 palette, void *destination, const u32 *image, u8 mode, int whole_image);
void GameSpriteImage_Blit(s16 x, s16 y, u8 width, u8 height, void *destination, u16 palette, u8 mode);
void GameSpriteImage_BlitScaled(s16 x, s16 y, u8 width, u8 height, void *destination,
    u16 palette, u8 mode, s32 scale_x, s32 scale_y);

static inline u8 GameSpriteImage_Width(const u32 *image, int index)
{
    u8 word = (index >> 3) + 1;
    u8 shift = (index & 7) * 4;
    u8 width = (image[word] >> shift) & 15;
    if (((image[0] & 0xf0) >> 4) >= 5)
        width |= (image[(u8)(word + ((image[0] & 0xff000000) >> 25))] >> shift) << 4;
    return width;
}
static inline u16 GameSpriteImage_HeightBlocks(const u32 *image) { return image[0] & 15; }
static inline u16 GameSpriteImage_WidthBlocks(const u32 *image) { return (image[0] & 0xf0) >> 4; }
void GameSpriteImage_Decode8x8(const u32 *source, u8 *destination);
void GameSpriteImage_Decode8x12(const u32 *source, u8 *destination);
void GameSpriteImage_Decode8x16(const u32 *source, u8 *destination);
void func_02023000(const u32 *source, u8 *destination);
void GameSpriteImage_Decode8x20(const u32 *source, u8 *destination);
void GameSpriteImage_Decode8x24(const u32 *source, u8 *destination);
void GameSpriteImage_Decode16x16(const u32 *source, u8 *destination);
void GameSpriteImage_Decode16x20(const u32 *source, u8 *destination);
void GameSpriteImage_Decode16x24(const u32 *source, u8 *destination);
#endif
