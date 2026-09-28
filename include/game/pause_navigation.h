#ifndef PIT_GAME_PAUSE_NAVIGATION_H
#define PIT_GAME_PAUSE_NAVIGATION_H

/*
 * Moving through the pause menu's pages and lists.
 */
#include <game/pause_scene.h>

typedef struct PauseRumbleDelayTask {
    u8 unknown_00[32];
    s32 phase, delay;
    u8 unknown_28[32];
} PauseRumbleDelayTask;

typedef struct PausePageTask {
    u8 unknown_00[32];
    s32 phase, delay, pending, unknown_2c, queued_direction;
    u8 unknown_34[20];
} PausePageTask;

typedef char PauseRumbleDelayTaskSize[sizeof(PauseRumbleDelayTask) == 72 ? 1 : -1];
typedef char PausePageTaskSize[sizeof(PausePageTask) == 72 ? 1 : -1];
typedef struct PauseMainMenuTask {
    u8 unknown_00[32];
    s32 phase, brightness, unknown_28, selection_delay;
    s32 queued_action, queued_movement, unknown_38, background_pending;
    u8 unknown_40[8];
} PauseMainMenuTask;
typedef char PauseMainMenuTaskSize[sizeof(PauseMainMenuTask) == 72 ? 1 : -1];

typedef struct PauseStatusPageTask {
    u8 unknown_00[32];
    s32 phase, blend, unknown_28, unknown_2c;
    union { s32 queued_direction, blend_step; };
    s32 previous_member;
    u8 unknown_38[16];
} PauseStatusPageTask;
typedef char StatusPageTaskSize[sizeof(PauseStatusPageTask) == 72 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
void PauseScene_DelayedRumbleTask(PauseRumbleDelayTask *task);
void PauseScene_ChooseEntryTask(PauseMenuElement *task);
void PausePage_CloseTask(PausePageTask *task);
void PausePage_OpenTask(PausePageTask *task);
void PauseMenu_UpdateTask(PauseMainMenuTask *task);
void PauseMenu_GetMemberPosition(int member, int *x, int *y);
int PauseMenu_CanSelectMember(int x, int y);
void PauseStatusPage_UpdateTask(PauseStatusPageTask *task);
void PauseStatusPage_Release(void);
void PauseStarPage_UpdateTask(PausePageTask *task);
#ifdef __cplusplus
}
#endif
#endif
