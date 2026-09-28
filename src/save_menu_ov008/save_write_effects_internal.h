#ifndef PIT_SAVE_WRITE_EFFECTS_INTERNAL_H
#define PIT_SAVE_WRITE_EFFECTS_INTERNAL_H
#include "save_menu_internal.h"

typedef struct SaveWriteBrightnessTask {
    u8 unknown_00[32];
    int phase, timer, amount;
    u8 unknown_2c[28];
} SaveWriteBrightnessTask;

typedef struct SaveWriteZoomTask {
    u8 unknown_00[32];
    int phase, timer, unknown_28, scale, blend;
    u8 unknown_34[20];
} SaveWriteZoomTask;

typedef struct SaveWritePanel {
    u16 slot, unknown_02;
    int x, y, target_x, target_y, scale, opacity, unknown_1c;
} SaveWritePanel;

/* Prefix of the shared save/load menu workspace, not its full allocation. */
typedef struct SaveWriteWorkPrefix {
    u8 unknown_000[0x13f];
    u8 unknown_13f;
    SaveWritePanel panels[2];
} SaveWriteWorkPrefix;

typedef struct SaveMenuExitTask {
    u8 unknown_00[32];
    int phase, timer, quit_to_title;
    u8 unknown_2c[28];
} SaveMenuExitTask;

typedef struct SaveMenuConfirmTask {
    u8 unknown_00[32];
    int state, counter, quit_to_title;
    u8 unknown_2c[16];
    int result;
    u8 unknown_40[8];
} SaveMenuConfirmTask;

#define WRITE_WORK (*(SaveWriteWorkPrefix *)data_ov008_02078290)

void SaveMenuWrite_StartEffects(void);
void SaveMenuWrite_StartRollback(void);
void SaveMenu_UpdateExit(SaveMenuExitTask *task);
void SaveMenu_UpdateConfirmation(SaveMenuConfirmTask *task);
void LoadMenu_UpdateCopyConfirmation(SaveMenuConfirmTask *task);
extern void func_ov008_0206b37c(SaveMenuTransferTask *);
extern void func_ov008_0206ac14(SaveMenuTransferTask *);
extern void func_ov008_0206ae78(SaveMenuTransferTask *);
extern void func_ov008_020727d8(int, int);
extern void func_ov005_02069bcc(int, int, int, int);

typedef char SaveWritePanelSizeCheck[sizeof(SaveWritePanel) == 32 ? 1 : -1];
typedef char SaveWriteWorkPrefixSizeCheck[sizeof(SaveWriteWorkPrefix) == 0x180 ? 1 : -1];
typedef char SaveMenuExitTaskSizeCheck[sizeof(SaveMenuExitTask) == 72 ? 1 : -1];
typedef char SaveMenuConfirmTaskSizeCheck[sizeof(SaveMenuConfirmTask) == 72 ? 1 : -1];

void SaveMenuWrite_UpdateBrightness(SaveWriteBrightnessTask *task);
void SaveMenuWrite_UpdateBackgroundZoom(SaveWriteZoomTask *task);

typedef char SaveWriteBrightnessTaskSizeCheck[
    sizeof(SaveWriteBrightnessTask) == 72 ? 1 : -1];
typedef char SaveWriteZoomTaskSizeCheck[
    sizeof(SaveWriteZoomTask) == 72 ? 1 : -1];
#endif
