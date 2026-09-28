#ifndef PIT_GAME_SAVE_MENU_H
#define PIT_GAME_SAVE_MENU_H
#include <game/task.h>
#include <game/save_data.h>
#include <game/text.h>

typedef struct SaveMenuPartyMember {
    u16 member_id;
    u16 base_max_hp, base_power, base_defense, base_speed, base_stache;
    u16 max_hp, current_hp, power, defense, speed, stache;
    SavePartyExperience experience;
    u8 unknown_1c[3];
    u8 equipment, badge;
    u8 reserved[3];
} SaveMenuPartyMember;
/* Display summaries preserve only the party and metadata fields used by menus. */
typedef struct SaveMenuSummary {
    SaveMenuPartyMember party[4];
    u8 present[4];
    u32 coins, play_time;
    u8 unknown_9c, unknown_9d, location, available;
} SaveMenuSummary;
typedef struct SaveMenuText {
    GameText text;
    u32 pixels[6144];
    u32 *owned_fonts[2];
    const u32 *fonts[20];
    u8 *strings[3];
} SaveMenuText;
typedef struct SaveMenuTransferTask {
    u8 unknown_00[32];
    int state, counter;
    int arguments[8];
} SaveMenuTransferTask;
/* The selection count is reused as the exit destination flag. */
typedef struct GameOverMenuTask {
    u8 unknown_00[32];
    int state, counter;
    union { int choice_count; int quit_to_title; };
    u8 unknown_2c[28];
} GameOverMenuTask;
typedef char GameOverMenuTaskSizeCheck[sizeof(GameOverMenuTask) == 72 ? 1 : -1];

typedef struct SaveMenuCursorTask {
    u8 unknown_00[16];
    SaveMenuTransferTask *parent;
    u8 unknown_14[12];
    int state, counter;
    u8 selection, unknown_29[3];
    int x, y;
    u8 unknown_34[20];
} SaveMenuCursorTask;
typedef char SaveMenuCursorTaskSizeCheck[sizeof(SaveMenuCursorTask) == 72 ? 1 : -1];

typedef struct SaveSceneTask {
    GameTask base;
    u8 unknown_18[16];
    GameIrqTask *irq;
    int phase;
} SaveSceneTask;
/* Motion prefix of a 72-byte menu task; the final word is not used here. */
typedef struct SaveMenuMotion {
    u8 unknown_00[36];
    int remaining;
    int unknown_28;
    int x, y, velocity_x, velocity_y, acceleration_x, acceleration_y;
} SaveMenuMotion;
typedef char SaveMenuMotionPrefixSizeCheck[sizeof(SaveMenuMotion) == 68 ? 1 : -1];

typedef char SaveMenuPartyMemberSizeCheck[sizeof(SaveMenuPartyMember) == 36 ? 1 : -1];
typedef char SaveMenuSummarySizeCheck[sizeof(SaveMenuSummary) == 160 ? 1 : -1];
typedef char SaveMenuTextSizeCheck[sizeof(SaveMenuText) == 24724 ? 1 : -1];
typedef char SaveSceneTaskSizeCheck[sizeof(SaveSceneTask) == 48 ? 1 : -1];

typedef struct SaveMenuScrollTask {
    u8 unknown_00[16];
    struct SaveMenuScrollTask *parent;
    u8 unknown_14[12];
    int state, counter;
    union {
        struct {
            s32 x, unknown_2c, width, source;
            s8 direction;
            u8 unknown_39[15];
        };
        struct {
            s32 part, anchor_x, anchor_y;
            u8 unknown_34[20];
        };
    };
} SaveMenuScrollTask;
typedef char SaveMenuScrollTaskSizeCheck[sizeof(SaveMenuScrollTask) == 72 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
void SaveMenuScroll_DrawArrow(SaveMenuScrollTask *task);
void SaveMenuScroll_DrawTextSegment(SaveMenuScrollTask *task);
void SaveMenuScroll_Update(SaveMenuScrollTask *task);
void SaveMenu_BuildStoredSummary(int slot, SaveMenuSummary *summary);
void SaveMenu_BuildLiveSummary(SaveMenuSummary *summary);
int SaveMenuText_DrawTextureRows(SaveMenuText *context, int *offset, int table, int entry, int width,
                                 int row);
int SaveMenuText_StreamObjectText(SaveMenuText *context, int engine, int *offset, int table, u16 entry);
int SaveMenuText_DrawObjectRows(SaveMenuText *context, int engine, int *offset, int table, u16 entry, int width, int row);
void SaveMenuText_DrawBackground(SaveMenuText *context, int engine, int background, int table, u16 entry,
                                 int x, int y);
void SaveMenuText_BuildDialog(SaveMenuText *text, int kind, int entry);
void SaveMenuText_PreparePanel(int panel, u16 entry);
SaveMenuTransferTask *SaveMenu_QueueLocationPanel(int panel, int image);
SaveMenuTransferTask *SaveMenu_CopyLocationPanel(int panel, u32 source_panel);
void SaveMenuText_ResetDialogSprites(void);
void SaveMenuText_ClearDialogVram(SaveMenuTransferTask *task);
void SaveMenuText_DrawSlotChoice(SaveMenuTransferTask *task);
void SaveMenuText_QueueTextureRow(int offset, int width, int row);
void SaveMenuText_QueueBackground(int engine, int background);
void SaveMenuText_QueueObjectStrip(int engine, int offset, int row, int column, int width);
u16 SaveMenuText_MeasureWidth(SaveMenuText *context, int table, int entry);
const u8 *SaveMenuText_GetString(SaveMenuText *context, int table, int entry);
SaveMenuText *SaveMenuText_Init(SaveMenuText *context);
void EraseMenu_ShowConfirmation(SaveMenuTransferTask *parent, int entry);
void EraseMenu_EnableConfirmationPlanes(SaveMenuCursorTask *task);
void EraseMenu_HideConfirmation(void);
void EraseMenu_DisableConfirmationPlanes(SaveMenuCursorTask *task);
void EraseMenu_CreateCursor(SaveMenuTransferTask *parent);
void EraseMenu_DrawCursor(SaveMenuCursorTask *task);
void GameOverMenu_CreateModels(int show_cursor);
void SaveMenu_CreateModels(void);
void LoadMenu_CreateModels(void);
int LoadMenuMotion_Update(SaveMenuMotion *motion);
int SaveMenuMotion_Update(SaveMenuMotion *motion);
void LoadMenuMotion_Initialize(SaveMenuMotion *motion, int x, int y);
void SaveMenuMotion_Initialize(SaveMenuMotion *motion, int x, int y);
SaveSceneTask *SaveScene_Init(SaveSceneTask *task, u32 priority, u32 unused);
SaveSceneTask *LoadScene_Init(SaveSceneTask *task, u32 priority, u32 unused);
SaveSceneTask *GameOverScene_Init(SaveSceneTask *task, u32 priority, u32 unused);
void SaveScene_InitGeometry(SaveSceneTask *unused);
void LoadScene_InitGeometry(SaveSceneTask *unused);
void GameOverScene_InitGeometry(SaveSceneTask *unused);
SaveSceneTask *SaveScene_Delete(SaveSceneTask *task);
SaveSceneTask *SaveScene_Destroy(SaveSceneTask *task);
SaveSceneTask *LoadScene_Delete(SaveSceneTask *task);
SaveSceneTask *LoadScene_Destroy(SaveSceneTask *task);
SaveSceneTask *GameOverScene_Delete(SaveSceneTask *task);
SaveSceneTask *GameOverScene_Destroy(SaveSceneTask *task);
GameIrqTask *SaveSceneIrq_Destroy(GameIrqTask *task);
GameIrqTask *SaveSceneIrq_Delete(GameIrqTask *task);
GameIrqTask *SaveSceneIrq_Init(GameIrqTask *task, u32 priority, u32 unused, void *parent);
GameIrqTask *LoadSceneIrq_Destroy(GameIrqTask *task);
GameIrqTask *LoadSceneIrq_Delete(GameIrqTask *task);
GameIrqTask *LoadSceneIrq_Init(GameIrqTask *task, u32 priority, u32 unused, void *parent);
GameIrqTask *GameOverSceneIrq_Destroy(GameIrqTask *task);
GameIrqTask *GameOverSceneIrq_Delete(GameIrqTask *task);
GameIrqTask *GameOverSceneIrq_Init(GameIrqTask *task, u32 priority, u32 unused, void *parent);
void SaveScene_UploadFrame(void);
void GameOverScene_UploadFrame(void);
GameIrqTask *EraseSceneIrq_Destroy(GameIrqTask *task);
GameIrqTask *EraseSceneIrq_Delete(GameIrqTask *task);
SaveSceneTask *EraseScene_Delete(SaveSceneTask *task);
SaveSceneTask *EraseScene_Destroy(SaveSceneTask *task);
SaveSceneTask *EraseScene_Init(SaveSceneTask *task, u32 priority, u32 unused);
void EraseScene_UploadFrame(void);
GameIrqTask *EraseSceneIrq_Init(GameIrqTask *task, u32 priority, u32 unused, void *parent);
void SaveMenu_FreeResources(void);
void SaveMenu_LoadResources(void);
void SaveScene_LoadResources(SaveSceneTask *task);
void EraseScene_LoadResources(SaveSceneTask *task);
void SaveMenu_LoadLocationName(int slot, int location);
void SaveMenu_LoadLocationImage(int slot, int location);
void SaveMenu_BuildInversePalette(int slot);
#ifdef __cplusplus
}
#endif
#endif
