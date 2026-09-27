#ifndef PIT_GAME_SHOP_SCENE_H
#define PIT_GAME_SHOP_SCENE_H

/*
 * The shop scene's state and lifecycle.
 */
#include <game/task.h>
typedef struct ShopSceneTask {
    GameTask base;
    u8 unknown_18[16];
    GameIrqTask *irq;
    int phase;
} ShopSceneTask;
typedef char ShopSceneTaskSizeCheck[sizeof(ShopSceneTask) == 48 ? 1 : -1];
#ifdef __cplusplus
extern "C" {
#endif
GameIrqTask *ShopSceneIrq_Destroy(GameIrqTask *task);
GameIrqTask *ShopSceneIrq_Delete(GameIrqTask *task);
GameIrqTask *ShopSceneIrq_Init(GameIrqTask *task, u32 priority, u32 unused, void *parent);
void ShopScene_LoadResources(ShopSceneTask *task);
ShopSceneTask *ShopScene_Init(ShopSceneTask *task, u32 priority, u32 unused);
ShopSceneTask *ShopScene_Delete(ShopSceneTask *task);
ShopSceneTask *ShopScene_Destroy(ShopSceneTask *task);
void ShopScene_LoadBackgrounds(ShopSceneTask *task);
void ShopScene_UploadFrame(void);
int ShopScene_CanSelectMember(int column, int row);
void ShopScene_GetMemberGridPosition(int member, int *column, int *row);
u32 ShopScene_GetSellBonus(ShopSceneTask *scene, u8 factor);
#ifdef __cplusplus
}
#endif
#endif
