#ifndef PIT_GAME_BATTLE_TEXT_H
#define PIT_GAME_BATTLE_TEXT_H

/*
 * The battle's two text renderers - clipped and tiled - and the flags that say
 * what still has to be uploaded for them.
 */
#include <game/text.h>

typedef union BattleTextUploadFlags {
    u8 raw;
    struct {
        u8 requested : 1, pending : 1, uploaded : 1, unknown : 5;
    } bits;
} BattleTextUploadFlags;
/* The upload word at +0x30 that the text views share with
   BattleInterfaceLayer: row_bytes holds the pending transfer size and
   resource_ready says the transfer finished. The layer record itself stays in
   battle_context.h, which includes this header. */
typedef union BattleInterfaceLayerFlags {
    u16 raw;
    struct {
        u16 row_bytes : 12;
        u16 resource_ready : 1;
        u16 notify_on_complete : 1;
        u16 clear_before_upload : 1;
        u16 state_15 : 1;
    } bits;
} BattleInterfaceLayerFlags;
typedef struct BattleTiledText {
    GameText text;
    BattleInterfaceLayerFlags text_flags;
    u16 allocation_size;
    u8 *buffer;
    u8 width, height, horizontal_mode, vertical_mode;
    BattleTextUploadFlags flags;
    u8 unknown_3d[3];
    u32 destination, palette;
} BattleTiledText;
typedef struct BattleClippedText {
    GameText text;
    BattleInterfaceLayerFlags text_flags;
    u16 allocation_size;
    u8 width, height;
    s8 horizontal_mode, vertical_mode;
    u8 unknown_38[2];
    u16 unknown_3a;
    s16 unknown_3c, clip_left, clip_right;
    u8 unknown_42[14];
    u32 destination, palette;
    BattleTextUploadFlags flags;
    u8 unknown_59[3];
} BattleClippedText;
typedef char BattleTextUploadFlags_SizeCheck[sizeof(BattleTextUploadFlags) == 1 ? 1 : -1];

typedef char BattleTiledText_SizeCheck[sizeof(BattleTiledText) == 72 ? 1 : -1];
typedef char BattleClippedText_SizeCheck[sizeof(BattleClippedText) == 92 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
void BattleTiledText_Init(BattleTiledText *text, void *buffer, s8 width, s8 height, s8 horizontal_mode,
                          s8 vertical_mode, u32 destination, u32 palette);
void BattleClippedText_Init(BattleClippedText *text, void *buffer, s8 width, s8 height, s8 horizontal_mode,
                            s8 vertical_mode, s16 clip_left, s16 clip_right, u32 destination, u32 palette);
void BattleTiledText_QueueResource(BattleTiledText *text, const void *resource, int clear);
void BattleClippedText_QueueResource(BattleClippedText *text, const void *resource, int clear, s16 value);
#ifdef __cplusplus
}
#endif
#endif
