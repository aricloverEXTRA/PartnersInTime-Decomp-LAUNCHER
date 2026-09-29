#ifndef PIT_GAME_PAUSE_SCENE_H
#define PIT_GAME_PAUSE_SCENE_H
#include <game/task.h>
typedef struct PauseMenuElement PauseMenuElement;
typedef struct PauseExitTask PauseExitTask;
typedef struct PauseSceneTask {
    GameTask task;
    u8 unknown18[20];
    GameIrqTask *irq;
    s32 phase;
    PauseMenuElement *menu;
} PauseSceneTask;
/* Separate scene named "Super Nawatobi Bros." by its embedded console title. */
typedef struct NawatobiSceneTask {
    GameTask task;
    u8 unknown18[16];
    GameIrqTask *irq;
    int phase;
} NawatobiSceneTask;
typedef char NawatobiSceneTaskSizeCheck[sizeof(NawatobiSceneTask) == 48 ? 1 : -1];
typedef char PauseSceneTaskSizeCheck[sizeof(PauseSceneTask) == 56 ? 1 : -1];
#ifdef __cplusplus
extern "C" {
#endif
GameIrqTask *PauseSceneIrq_Destroy(GameIrqTask *task);
GameIrqTask *PauseSceneIrq_Delete(GameIrqTask *task);
PauseSceneTask *PauseScene_Destroy(PauseSceneTask *task);
PauseSceneTask *PauseScene_Delete(PauseSceneTask *task);
void NawatobiScene_LoadResources(NawatobiSceneTask *);
NawatobiSceneTask *NawatobiScene_Delete(NawatobiSceneTask *task);
NawatobiSceneTask *NawatobiScene_Destroy(NawatobiSceneTask *task);
NawatobiSceneTask *NawatobiScene_Init(NawatobiSceneTask *task, u32 priority, u32 unused);
void NawatobiScene_UploadFrame(void);
GameIrqTask *NawatobiSceneIrq_Destroy(GameIrqTask *task);
GameIrqTask *NawatobiSceneIrq_Delete(GameIrqTask *task);
GameIrqTask *NawatobiSceneIrq_Init(GameIrqTask *task, u32 priority, u32 unused, void *parent);
void PauseScene_LoadResources(PauseSceneTask *task);
void PauseScene_PrepareExitTask(PauseExitTask *task);
void PauseScene_FadeOutTask(PauseExitTask *task);
void PauseScene_RequestExit(PauseSceneTask *scene, int mode);
/* Native callers may pass the scene, although progress is global. */
s32 PauseTransition_GetProgress(PauseSceneTask *unused_scene);
PauseSceneTask *PauseScene_Init(PauseSceneTask *task, u32 priority, u32 unused);
GameIrqTask *PauseSceneIrq_Init(GameIrqTask *task, u32 priority, u32 unused, void *parent);
#ifdef __cplusplus
}
#endif
#endif
