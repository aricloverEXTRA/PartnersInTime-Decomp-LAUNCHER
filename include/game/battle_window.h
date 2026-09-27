#ifndef PIT_GAME_BATTLE_WINDOW_H
#define PIT_GAME_BATTLE_WINDOW_H

/*
 * The battle's message window: its manager, the animator that draws it and the
 * queued transfer tasks that upload its tiles and tilemap.
 */

#include <game/window.h>
#include <game/texture_allocation.h>

#ifdef __cplusplus
/* Prefix of the virtual interface shared by the battle window managers. */
struct BattleWindowInterface {
    virtual void unknown_00();
    virtual void unknown_04();
    virtual void unknown_08();
    virtual void unknown_0c();
    virtual int open(GameWindowProperties *, s16);
    virtual void close(s16);
};
#endif

typedef struct BattleWindowState {
    u8 unknown_00[8];
    /* Embedded motion prefix, not a separately allocated scene object. */
    u8 motion_state[188];
    const void *upload_source;
    void *upload_destination;
    u8 upload_columns, upload_rows, upload_pitch;
    u8 unknown_cf;
    u8 unknown_d0[68];
} BattleWindowState;

typedef struct BattleWindowTransferTask {
    void *callback;
    union {
        const void *source;
        u32 fill_value;
    };
    void *destination;
    u32 size;
} BattleWindowTransferTask;

typedef struct BattleWindowManager {
    GameWindowManager base;
    u32 unknown_f44;
    GameOamEntry oam[2][32];
    GameTexturePalette palettes[2];
    u32 texture_offset;
    const u32 *texture_banks;
    GameSpritePalette sprite_palette;
    u32 flags;
    BattleWindowState windows[4];
} BattleWindowManager;

typedef struct BattleWindowAnimator {
    GameSpriteAnimator base;
    BattleWindowManager *manager;
    u32 texture_offset;
    u32 texture_cursor;
    u32 palette_offset;
    u32 unknown_7c, unknown_80;
} BattleWindowAnimator;

typedef char BattleWindowManagerSizeCheck[sizeof(BattleWindowManager) == 5608 ? 1 : -1];
typedef char BattleWindowAnimatorSizeCheck[sizeof(BattleWindowAnimator) == 132 ? 1 : -1];
typedef char BattleWindowStateSizeCheck[sizeof(BattleWindowState) == 276 ? 1 : -1];
typedef char BattleWindowState_MotionOffsetCheck[
    (u32)&((BattleWindowState *)0)->motion_state == 8 ? 1 : -1];
typedef char BattleWindowManager_WindowsOffsetCheck[
    (u32)&((BattleWindowManager *)0)->windows == 4504 ? 1 : -1];
typedef char BattleWindowTransferTaskSizeCheck[sizeof(BattleWindowTransferTask) == 16 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
void BattleWindow_Close(BattleWindowManager *manager, s16 index);
void BattleWindow_AllocatePool(BattleWindowManager *manager, int heap);
void BattleWindow_AllocateSpritePalette(BattleWindowManager *manager);
void BattleWindow_AllocateTexturePalettes(BattleWindowManager *manager);
void BattleWindow_Update(BattleWindowManager *manager);
BattleWindowManager *BattleWindow_Delete(BattleWindowManager *manager);
BattleWindowManager *BattleWindow_Destroy(BattleWindowManager *manager);
BattleWindowManager *BattleWindow_Construct(BattleWindowManager *manager, int priority, int unused,
    GameSpriteAnimator *animator, u32 texture_offset, const GameWindowBuffers *buffers,
    const GameWindowTilemaps *tilemaps, int configure);
void BattleWindowAnimator_Draw(BattleWindowAnimator *animator, GameSpriteAnimation *animation);
void BattleWindowAnimator_CopySubOam(BattleWindowAnimator *animator, GameOamEntry *oam);
BattleWindowAnimator *BattleWindowAnimator_Init(BattleWindowAnimator *animator);
void BattleWindow_FillTextureTask(BattleWindowTransferTask *task);
void BattleWindow_CopyTextureTask(BattleWindowTransferTask *task);
void BattleWindow_ResetSubScrollTask(BattleWindowTransferTask *task);
void BattleWindow_UploadSubWindowTask(BattleWindowTransferTask *task);
void BattleWindow_UploadSubTilemapTask(BattleWindowTransferTask *task);
void BattleWindow_UploadTextTilesTask(BattleWindowTransferTask *task);
#ifdef __cplusplus
}
#endif
#endif
