#ifndef PIT_SAVE_MENU_INTERNAL_H
#define PIT_SAVE_MENU_INTERNAL_H
#include <game/save_menu.h>
#include <game/save_storage.h>
#include <game/script_vm.h>
#include <game/heap.h>
#include <game/input.h>
#include <game/sprite_output.h>
#include <game/overlay005_display.h>
#include <game/overlay005_archive.h>
#include <nitro/gx_init.h>
#include <nitro/gx_projection.h>
typedef SaveMenuTransferTask MenuElement;
/* Shared save/load workspace prefix through the panel selection. */
typedef struct SaveMenuEntryWorkPrefix {
    SaveMenuSummary summaries[2];
    u8 unknown_140[0x50];
    u8 menu_mode, exit_mode, unknown_192[6], scroll_locked;
    s8 selection, previous_selection;
    union { u8 message_visible; s8 cursor_hidden; };
    s8 input_locked, selected_panel;
    union {
        u8 unknown_19e[2];
        struct { s8 copy_destination; u8 unknown_19f; };
    };
} SaveMenuEntryWorkPrefix;
typedef char SaveMenuEntryWorkPrefixSizeCheck[
    sizeof(SaveMenuEntryWorkPrefix) == 0x1a0 ? 1 : -1];
/* Cursor anchors shared by the save, load and game-over menus. Save choices
 * use three row coordinates; load choices use a fixed step from the first. */
typedef struct SaveMenuCursorWorkPrefix {
    SaveMenuEntryWorkPrefix menu;
    int confirmation_x, confirmation_y, action_x;
    union { int action_y; int action_rows[3]; };
} SaveMenuCursorWorkPrefix;
typedef char SaveMenuCursorWorkPrefixSizeCheck[
    sizeof(SaveMenuCursorWorkPrefix) == 0x1b8 ? 1 : -1];
typedef char SaveMenuCursorWorkAnchorOffsetCheck[
    (u32)&((SaveMenuCursorWorkPrefix *)0)->confirmation_x == 0x1a0 ? 1 : -1];
/* Native cursor code also keeps this interior row-table address as a literal. */
extern int data_ov008_0207843c[3];
/* ResourceA owns a 336-byte pool slot; cursor updates only use this prefix. */
typedef struct SaveMenuCursorSpritePrefix {
    u8 unknown_00[92];
    s16 x, y;
} SaveMenuCursorSpritePrefix;
typedef char SaveMenuCursorSpritePrefixSizeCheck[
    sizeof(SaveMenuCursorSpritePrefix) == 96 ? 1 : -1];

/* Prefix through the two 14-byte panel rows at workspace offset 0x27E2. */
typedef struct SaveMenuPanelValuesWorkPrefix {
    SaveMenuEntryWorkPrefix menu;
    u8 unknown_1a0[0x2642];
    u8 panel_values[2][14];
} SaveMenuPanelValuesWorkPrefix;
typedef char SaveMenuPanelValuesWorkPrefixSizeCheck[
    sizeof(SaveMenuPanelValuesWorkPrefix) == 0x2800 ? 1 : -1];

#define REG16(address) (*(vu16 *)(address))
#define REG32(address) (*(vu32 *)(address))
#define MENU_ARCHIVE (*(Overlay5Archive **)(data_ov005_0206a180 + 44))
#define MENU_OAM_BUFFERS (*(u8 **)(data_ov005_0206a180 + 40))
#include <game/equipment.h>
extern SaveMenuText *data_ov008_0207828c;
extern u8 data_ov005_0206a180[];
extern const char *data_ov008_020780bc[4];
extern void func_ov005_02067468(Overlay5Archive *, u8, const void *, int);
extern void *Overlay5Archive_ReadEntry(Overlay5Archive *, u8, int, u32 *, int);
extern void func_ov005_02067424(void *, u8);
extern void *func_ov005_0206687c(const char *, u32 *, int);
extern SaveMenuTransferTask *func_ov005_0206659c(void (*)(SaveMenuTransferTask *), int, int);
extern void func_ov005_0206650c(SaveMenuTransferTask *);
extern void func_0203b7b4(const void *, void *, u32);
extern void func_0203b7a0(u32, void *, u32);
extern void GameResource_Move16(const void *, void *, u32);
extern void func_0202cf04(const void *, void *, int, int);
extern void DC_FlushRange(const void *, u32);
extern void func_02035818(void);
extern void func_02035c00(int);
extern u8 data_0206032c[];
extern void func_ov005_02066128(int);
extern void func_ov005_0206781c(int, int);
extern void func_ov005_02066638(int);
extern void func_ov005_020690f0(int);
extern void func_ov005_02069b54(void);
extern void func_ov005_02068ca8(int);
extern void func_ov005_02069674(int);
extern void func_ov005_02069998(int);
extern Overlay5Archive *func_ov005_02067584(Overlay5Archive *, int, int, void *, int);
extern void func_02009078(int, void *, void *);
extern void func_02036988(vu16 *, int, int, int, int);
extern void func_02037190(void);
extern void func_02037024(void);
extern void func_02037108(void);
extern void func_02036ee0(int, int, int, int, int);
extern void func_02035fd0(int);
extern void func_0203613c(int);
extern void func_02035ebc(int);
extern void func_02035b0c(int);
extern void func_020359c4(int);
extern void func_02035a40(int);
extern void func_02035938(int);
extern GameTaskVTable data_ov008_02077f24;
extern SaveSceneTask *data_ov008_02078280;
extern void SaveScene_LoadResources(SaveSceneTask *);
extern void SaveMenu_UpdateEntry(MenuElement *);
extern void SaveMenu_UpdateSelection(MenuElement *);
extern GameTaskVTable data_ov008_02077f8c;
extern SaveSceneTask *data_ov008_02078284;
extern void func_ov008_0206f588(SaveSceneTask *);
extern void func_ov008_0206e9a0(MenuElement *);
void LoadMenu_InitializePanels(void);
void LoadMenu_UpdateSelection(MenuElement *);
void LoadMenu_UpdateExit(MenuElement *);
extern GameTaskVTable data_ov008_02077ff0;
extern SaveSceneTask *data_ov008_02078288;
extern void GameOverScene_LoadResources(SaveSceneTask *);
void GameOverMenu_UpdateEntry(MenuElement *);
void GameOverMenu_UpdateSelection(MenuElement *);
void GameOverMenu_UpdateExit(MenuElement *);
extern void func_ov005_020671e4(Overlay5Archive *);
extern void func_ov005_02065f10(void);
extern void func_02009058(int);
extern void func_ov005_02069928(void);
extern void func_ov005_02069660(void);
extern void func_ov005_02068c94(void);
extern void func_ov005_020699f0(void);
extern void func_ov005_020690dc(void);
extern void func_ov005_02066624(void);
extern GameTaskVTable data_ov008_02077f10;
extern GameTaskVTable data_ov008_02077f78;
extern GameTaskVTable data_ov008_02077fdc;
extern int func_ov005_02065f00(void);
extern void func_ov005_0206651c(int, int);
extern void func_ov005_0206642c(int, int);
extern void func_ov005_02068f84(int);
extern void func_ov005_02065eec(void);
extern u8 data_ov008_02078290[];

extern void func_ov008_02075ac8(SaveMenuTransferTask *);
extern void SaveMenuText_UploadBackground(SaveMenuTransferTask *);
extern void SaveMenuText_UploadObjectStrip(SaveMenuTransferTask *);
extern GameTaskVTable data_ov008_02078264, data_ov008_02078250;
extern SaveSceneTask *data_ov008_0207aa98;
extern u8 data_02059f44, data_02059f48;
extern void EraseScene_LoadResources(SaveSceneTask *);
extern void func_ov008_02076c70(MenuElement *);
extern void func_ov008_02075dc8(SaveMenuText *);
static inline void SaveMenuText_DeleteOwned(SaveMenuText *text)
{
    if (text) {
        func_ov008_02075dc8(text);
        GameHeap_Delete(text);
    }
}
#endif
