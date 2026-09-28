/*
 * Session task (ARM9 resident, 0x02005D74-0x020063A4).
 *
 * Owns the running session and its state requests. Reserves and loads each
 * state's overlay, then disables itself while the child scene runs.
 */

#include <game/session.h>
#include <game/save_data.h>
#include <game/heap.h>
#include <game/archive_io.h>
#include <game/field_system.h>
#include <game/battle_frame.h>
#include <game/title_startup.h>

extern GameTaskVTable data_02050020;
extern void *data_02059f64;
extern void func_02004f9c(u32 language);

extern GameTask *func_020290a8(void);
extern u8 data_0205002c[];

FS_EXTERN_OVERLAY(ov000);
FS_EXTERN_OVERLAY(ov002);
FS_EXTERN_OVERLAY(ov005);
FS_EXTERN_OVERLAY(ov006);
FS_EXTERN_OVERLAY(ov007);
FS_EXTERN_OVERLAY(ov008);
FS_EXTERN_OVERLAY(ov009);
/* End of code and BSS for each overlay; values come from the EUR table. */
extern u8 GameOverlay_FieldEnd[];
extern u8 GameOverlay_BattleEnd[];
extern u8 GameOverlay_TitleEnd[];
extern u8 GameOverlay_SceneEnd[];
extern u8 GameOverlay_SaveMenuEnd[];
extern u8 GameOverlay_ShopEnd[];

typedef struct SessionTransitionFlags {
    u8 previous[0x514];
    u8 reserved_0_2 : 3, reset_on_field : 1, reserved_4_7 : 4;
} SessionTransitionFlags;

GameSessionTask *GameSessionTask_Init(GameSessionTask *task, u32 priority, u32 unused, void *argument,
                             u32 requested_state, u32 unknown, const GameSessionSettings *settings)
{
    GameTask_Init(&task->base, priority, unused, argument);
    task->base.vtable = &data_02050020;
    func_02004f9c(settings->language);
    GameSession_InitializeSaveContext(settings);
    *(void **)(gSaveData + 0x51c) = data_02059f64;
    GameSessionTask_RequestState(task, requested_state);
    return task;
}

GameSessionTask *GameSessionTask_Destroy(GameSessionTask *task)
{
    task->base.vtable = &data_02050020;
    GameTask_DestroyBase(&task->base);
    return task;
}

GameSessionTask *GameSessionTask_Delete(GameSessionTask *task)
{
    task->base.vtable = &data_02050020;
    GameTask_DestroyBase(&task->base);
    GameHeap_Delete(task);
    return task;
}

static inline void GameSession_PrepareOverlay(u8 state)
{
    switch (state) {
    case 0:
        GameHeap_RebaseMain((u32)GameOverlay_FieldEnd);
        ArchiveIO_BeginGlobalOverlay(FS_OVERLAY_ID(ov000), 3, 0);
        break;
    case 1:
        GameHeap_RebaseMain((u32)GameOverlay_BattleEnd);
        ArchiveIO_BeginGlobalOverlay(FS_OVERLAY_ID(ov002), 3, 0);
        break;
    case 2:
        GameHeap_RebaseMain((u32)GameOverlay_TitleEnd);
        ArchiveIO_LoadOverlay(FS_OVERLAY_ID(ov005), 0);
        ArchiveIO_BeginGlobalOverlay(FS_OVERLAY_ID(ov006), 3, 0);
        break;
    case 3:
        GameHeap_RebaseMain((u32)GameOverlay_TitleEnd);
        ArchiveIO_LoadOverlay(FS_OVERLAY_ID(ov005), 0);
        ArchiveIO_BeginGlobalOverlay(FS_OVERLAY_ID(ov006), 3, 0);
        break;
    case 4:
        GameHeap_RebaseMain((u32)GameOverlay_SceneEnd);
        ArchiveIO_LoadOverlay(FS_OVERLAY_ID(ov005), 0);
        ArchiveIO_BeginGlobalOverlay(FS_OVERLAY_ID(ov007), 3, 0);
        break;
    case 5: case 7: case 8: case 9:
        GameHeap_RebaseMain((u32)GameOverlay_SaveMenuEnd);
        ArchiveIO_LoadOverlay(FS_OVERLAY_ID(ov005), 0);
        ArchiveIO_BeginGlobalOverlay(FS_OVERLAY_ID(ov008), 3, 0);
        break;
    case 6:
        GameHeap_RebaseMain((u32)GameOverlay_ShopEnd);
        ArchiveIO_LoadOverlay(FS_OVERLAY_ID(ov005), 0);
        ArchiveIO_BeginGlobalOverlay(FS_OVERLAY_ID(ov009), 3, 0);
        break;
    case 10:
        GameHeap_RebaseMain((u32)GameOverlay_TitleEnd);
        ArchiveIO_LoadOverlay(FS_OVERLAY_ID(ov005), 0);
        ArchiveIO_BeginGlobalOverlay(FS_OVERLAY_ID(ov006), 3, 0);
        break;
    }
}

static inline void GameSession_CreateStateTask(GameSessionTask *task)
{
    switch (task->requested_state) {
    case 0:
        {
            FieldSystem *system;
            if (((SessionTransitionFlags *)gSaveData)->reset_on_field) GameSession_ResetSaveState();
            system = GameHeap_New(sizeof(*system), 0, data_0205002c, 0);
            if (system) FieldSystem_Init(system, 8, (u32)data_0205002c, task);
        }
        break;
    case 1: BattleMain_Create(0); break;
    case 2: GameTask_CreateTitleScreen(); break;
    case 3: TitleAnimation_Create(); break;
    case 4: GameTask_CreateSceneController(); break;
    case 5: GameTask_CreateSaveMenu(); break;
    case 6: GameTask_CreateShopMenu(); break;
    case 7: GameTask_CreateGameOver(); break;
    case 8: GameTask_CreateLoadMenu(); break;
    case 9: func_020290a8(); break;
    case 10: GameTask_CreateStaffCredits(); break;
    }
}

void GameSessionTask_Update(GameSessionTask *task)
{
    switch (task->base.status) {
    case 0:
        GameSession_PrepareOverlay(task->requested_state);
        task->base.status = 1;
        return;
    case 1:
        if (ArchiveIO_FinishGlobalOverlay()) return;
        GameTask_Disable(&task->base);
        GameSession_CreateStateTask(task);
        task->base.status = 4;
        return;
    case 2:
        GameSession_PrepareOverlay(task->requested_state);
        task->base.status = 3;
        return;
    case 3:
        if (ArchiveIO_FinishGlobalOverlay()) return;
        GameTask_Disable(&task->base);
        GameSession_CreateStateTask(task);
        task->base.status = 4;
        break;
    case 4:
        /* The child state owns execution until another transition request. */
        break;
    }
}

void GameSessionTask_RequestState(GameSessionTask *task, u32 requested_state)
{
    task->requested_state = requested_state;
    task->base.status = 0;
    GameTask_Enable(&task->base);
}

void GameSessionTask_RequestStatePhase2(GameSessionTask *task, u32 requested_state)
{
    task->requested_state = requested_state;
    task->base.status = 2;
    GameTask_Enable(&task->base);
}
