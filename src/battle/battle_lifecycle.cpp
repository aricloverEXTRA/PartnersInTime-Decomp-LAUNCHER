/*
 * Battle lifecycle (overlay 2, 0x02077268-0x02077C38).
 *
 * Creating the battle task, its VBlank handler and its teardown, including the
 * texture upload that runs inside the blanking period and the models, sprite and
 * matrix animations released at the end.
 */

#include <game/battle_common_resources.h>
extern "C" {
#include <game/battle_frame.h>
#include "battle_scheduler_internal.h"
#include <game/battle_entry.h>
#include <game/battle_context.h>
#include <game/save_data.h>
#include <game/heap.h>
#include <game/session.h>
#include <game/input.h>
#include <game/graphics_resource.h>
#include <nitro/gx_init.h>
void func_02009058(int);
void func_0202cbd4(void *, int, u32);
void func_0202cc58(const void *, void *, u32);
void func_0202cd2c(const void *, void *, u32);
void func_0202ce08(const void *, void *, int, int);
void *func_02036650(void);
void func_02036988(vu32 *, int, int, int, int);
void func_ov002_0206a008(u8 *);
void BattleModel_StopAlternate(BattleModel *);
void BattleModel_StopPrimary(BattleModel *);
void *BattleTransfer_EnqueueBeforeMapping(int (*)(void *), void *, int, int);
extern GameSessionTask *data_02059ffc;
extern int data_ov002_020c071c;
extern u8 data_ov002_020beb24[], data_ov002_020beb30[];
u32 GX_ResetBankForTex(void);
u32 func_02035610(void);
u32 GX_ResetBankForSubOBJ(void);
u32 GX_ResetBankForBG(void);
u32 GX_ResetBankForTexPltt(void);
u32 GX_ResetBankForSubBG(void);
u32 GX_ResetBankForSubOBJExtPltt(void);
}
#define FRAME ((BattleFrameContextView *)gBattleContext)
#define HEAPS ((BattleEntryHeaps *)(gBattleContext + 57672))
#define BUFFER (*(void **)(gBattleContext + 57696))
#define ENTRY ((BattleEntrySaveView *)(gSaveData + 0x558))
#define REG32(address) (*(vu32 *)(address))

/* These inline cleanup guards preserve the native nullable deletion calls. */
static inline void DeleteModel(BattleModel *model)
{
    if (model)
        model->unknown_14(); /* Deleting virtual slot at vtable +0x14. */
}
static inline void DeleteSpriteAnimation(GameSpriteAnimation *animation)
{
    if (animation) {
        GameSpriteAnimation_Finalize(animation);
        GameHeap_Delete(animation);
    }
}
static inline void DeleteMatrixAnimation(GameMatrixAnimation *animation)
{
    if (animation) {
        GameMatrixAnimation_Finalize(animation);
        GameHeap_Delete(animation);
    }
}
static inline void SetScroll(vu32 *reg, u16 x, u16 y)
{
    *reg = (x & 0x1FF) | ((y << 16) & 0x1FF0000);
}
static inline void CopyMemory(const void *source, void *destination, u32 size)
{
    if (source < destination)
        func_0202cd2c(source, destination, size);
    else
        func_0202cc58(source, destination, size);
}

/* Function order follows MWCC reverse emission. */
extern "C" BattleFrameContextView *BattleMain_Create(u32 heap_start)
{
    /* Main-heap storage starts beyond the resident battle overlay allocation. */
    if (heap_start < 0x020CBFE0)
        heap_start = 0x020CBFE0;
    GameHeap_RebaseMain(heap_start);
    BattleScheduler *system = (BattleScheduler *)GameHeap_New(sizeof(BattleScheduler), 0, data_ov002_020beb24, 0);
    if (system)
        BattleScheduler_Init(system, 8, (u32)data_ov002_020beb24);
    BattleFrameContextView *battle =
        (BattleFrameContextView *)GameHeap_New(sizeof(BattleFrameContextView), 0, data_ov002_020beb30, 0);
    if (battle)
        battle = BattleMain_InitTask(battle, 0);
    return battle;
}

extern "C" BattleFrameContextView *BattleMain_InitTask(BattleFrameContextView *battle, int priority)
{
    BattleSchedulerNode_Insert((BattleSchedulerNode *)battle, priority);
    gBattleContext = (u8 *)battle;
    battle->update = BattleMain_Initialize;
    return battle;
}

extern "C" void *BattleMain_Destroy(void *task)
{
    func_02009058(0);
    func_02009058(1);
    GameTexturePalette_ReleaseBuffer(0);
    BattleCommonResources_ReleaseGraphics(gBattleContext);
    if (gBattleContext) {
        BattleObjectData_ResetAll();
        for (u16 id = 60; id < 68; ++id) {
            BattleEnemyActor *enemy = (BattleEnemyActor *)BattleActor_GetEnemySlot(id);
            if (enemy->defeat_effect_kind == 4) {
                BattleSceneObject *scene = enemy->actor.scene_object;
                if (scene && scene->primary_model)
                    scene->primary_model = 0;
            }
        }
        for (int i = 0; i < 68; ++i) {
            if (FRAME->scene[i].alternate_model) {
                BattleModel_StopAlternate(FRAME->scene[i].alternate_model);
                DeleteModel(FRAME->scene[i].alternate_model);
            }
            if (FRAME->scene[i].primary_model) {
                BattleModel_StopPrimary(FRAME->scene[i].primary_model);
                DeleteModel(FRAME->scene[i].primary_model);
            }
        }
        for (int i = 0; i < 72; ++i) {
            if (FRAME->resource_models[i]) {
                DeleteModel(FRAME->resource_models[i]);
                FRAME->resource_models[i] = 0;
            }
        }
        if (FRAME->party_animation_model) {
            DeleteModel(FRAME->party_animation_model);
            FRAME->party_animation_model = 0;
        }
        GameTexturePalette_Unlink((GameTexturePalette *)(gBattleContext + 26844));
        GameTexturePalette_Unlink((GameTexturePalette *)(gBattleContext + 52100));
        if (FRAME->sprite_animation)
            DeleteSpriteAnimation(FRAME->sprite_animation);
        if (FRAME->matrix_animation)
            DeleteMatrixAnimation(FRAME->matrix_animation);
        if (BUFFER) {
            GameHeap_Free(BUFFER);
            BUFFER = 0;
        }
        if (HEAPS->main_data)
            GameHeap_Destroy(HEAPS->main_data);
        if (HEAPS->sub_data)
            GameHeap_Destroy(HEAPS->sub_data);
        if (HEAPS->scene_data)
            GameHeap_Destroy(HEAPS->scene_data);
        if (HEAPS->attack_data)
            GameHeap_Destroy(HEAPS->attack_data);
        if (HEAPS->texture_animation)
            GameHeap_Destroy(HEAPS->texture_animation);
        if (HEAPS->parameter_animation)
            GameHeap_Destroy(HEAPS->parameter_animation);
    }
    data_ov002_020c071c = 0;
    REG32(0x04000000) &= ~0x1F00;
    REG32(0x04001000) &= ~0x1F00;
    REG32(0x04000010) = 0;
    REG32(0x04000014) = 0;
    REG32(0x04000018) = 0;
    REG32(0x0400001C) = 0;
    REG32(0x04001010) = 0;
    REG32(0x04001014) = 0;
    REG32(0x04001018) = 0;
    REG32(0x0400101C) = 0;
    GX_ResetBankForTex();
    func_02035610();
    GX_ResetBankForSubOBJ();
    GX_ResetBankForBG();
    GX_ResetBankForTexPltt();
    GX_ResetBankForSubBG();
    GX_ResetBankForSubOBJExtPltt();
    gBattleContext = 0;
    if (ENTRY->exit_mode == 1 && !(ENTRY->flags.unknown_00_02 & 2)) {
        func_0202cbd4(ENTRY, 0, sizeof(BattleEntrySaveView));
        GameSessionTask_RequestStatePhase2(data_02059ffc, 7);
    } else {
        GameSessionTask_RequestStatePhase2(data_02059ffc, 0);
    }
    BattleSchedulerNode_Unlink((BattleSchedulerNode *)task);
    return task;
}

extern "C" void BattleMain_VBlank(void)
{
    if (!gBattleContext)
        return;
    GXi_SetMasterBrightness((vu16 *)0x0400006C, FRAME->main_brightness_level / 2 - 16);
    GXi_SetMasterBrightness((vu16 *)0x0400106C, FRAME->sub_brightness_level / 2 - 16);
    func_02036988((vu32 *)0x04000050, (u16)FRAME->render_values_28[3], (u16)FRAME->render_values_28[2],
                  (u16)FRAME->render_values_28[1], (u16)FRAME->render_values_28[0]);
    /* Backgrounds 2 and 3 combine camera parallax and accumulated scroll in 1/32 pixels. */
    SetScroll((vu32 *)0x04000014, FRAME->view_position[0] + 128, FRAME->view_position[1] + 32);
    SetScroll((vu32 *)0x04000018,
              (FRAME->scroll_position[0] + FRAME->view_position[0] * FRAME->parallax_scale[0]) / 32 + 128,
              (FRAME->scroll_position[1] + FRAME->view_position[1] * FRAME->parallax_scale[1]) / 32 + 32);
    SetScroll((vu32 *)0x0400001C,
              (FRAME->scroll_position[2] + FRAME->view_position[0] * FRAME->parallax_scale[2]) / 32 + 128,
              (FRAME->scroll_position[3] + FRAME->view_position[1] * FRAME->parallax_scale[3]) / 32 + 32);
    if (FRAME->palette[0]) {
        CopyMemory(FRAME->palette[0], (void *)0x05000000, 512);
        FRAME->palette[0] = 0;
    }
    if (!((BattleContext *)gBattleContext)->runtime.flags.bits.unknown_06)
        func_ov002_0206a008(gBattleContext);
    GameDisplay_CopyOam(3);
    GameDisplay_ClearOam();
    func_02036988((vu32 *)0x04001050, (u16)FRAME->render_values_30[3], (u16)FRAME->render_values_30[2],
                  (u16)FRAME->render_values_30[1], (u16)FRAME->render_values_30[0]);
    SetScroll((vu32 *)0x04001014, FRAME->view_position[2] + 128, FRAME->view_position[3] + 32);
    SetScroll((vu32 *)0x04001018,
              (FRAME->scroll_position[4] + FRAME->view_position[2] * FRAME->parallax_scale[4]) / 32 + 128,
              (FRAME->scroll_position[5] + FRAME->view_position[3] * FRAME->parallax_scale[5]) / 32 + 32);
    SetScroll((vu32 *)0x0400101C,
              (FRAME->scroll_position[6] + FRAME->view_position[2] * FRAME->parallax_scale[6]) / 32 + 128,
              (FRAME->scroll_position[7] + FRAME->view_position[3] * FRAME->parallax_scale[7]) / 32 + 32);
    if (FRAME->palette[1]) {
        CopyMemory(FRAME->palette[1], (void *)0x05000400, 480);
        FRAME->palette[1] = 0;
    }
    /* The final sixteen sub-screen colors have a separate pending source. */
    if (FRAME->sub_palette_tail) {
        CopyMemory(FRAME->sub_palette_tail, (void *)0x050005E0, 32);
        FRAME->sub_palette_tail = 0;
    }
    /* Convert the captured linear image into eight-row tile groups. */
    if (FRAME->image_effect_frames) {
        func_0202ce08((void *)0x06898000, (u8 *)func_02036650() + 0x2000, 128, 192);
    }
    ++FRAME->frame;
}

extern "C" int BattleMain_UploadTextures(void *task)
{
    BattleRenderModels_UpdateTextures(((BattleFrameContextView *)gBattleContext)->texture_banks, 1);
    GameTexturePalette_Upload(((BattleFrameContextView *)gBattleContext)->palette_banks);
    return (int)BattleTransfer_EnqueueBeforeMapping(BattleMain_UploadTextures, 0, 0, 0);
}
