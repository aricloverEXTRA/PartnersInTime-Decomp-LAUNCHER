#ifndef PIT_PAUSE_SCENE_INTERNAL_H
#define PIT_PAUSE_SCENE_INTERNAL_H
#include <game/pause_scene.h>
extern "C" {
#include <game/heap.h>
#include <game/save_data.h>
#include <game/input.h>
#include <game/overlay005_display.h>
#include <game/overlay005_archive.h>
#include <game/sprite_output.h>
#include <game/palette_animation.h>
#include <nitro/gx_init.h>
}
extern "C" {
#include <game/rumble.h>
#include <game/scene_script.h>
}
struct Overlay5DisplayResources {
    u8 unknown000[40];
    void *oam;
    Overlay5Archive *archive;
    GameSpriteAllocation allocations[2];
    GameSpritePalette palettes[4];
};
typedef char DisplayResourcesSizeCheck[sizeof(Overlay5DisplayResources) == 176 ? 1 : -1];
struct PausePanel {
    virtual void destroy();
    virtual void deleteInstance();
};
struct PauseSceneWork {
    u8 unknown00[0x1e];
    u16 input_mask;
    u8 unknown20[4];
    s16 saved_party_state;
    u16 unknown026;
    void *owned28, *owned2c, *image30, *palette34;
    u8 unknown038[40];
    void *image60, *palette64;
    u8 unknown068[24];
    void *owned80;
    u8 initialized, tutorial, background_dirty, unknown087;
    void *owned88, *owned8c, *owned90, *owned94;
    GameSpritePalette palette98, paletteac;
    void *ownedc0, *background_tiles[2], *background_maps[2], *secondary_background_maps[2],
        *number_glyph_tiles, *number_glyph_pixels, *number_scratch;
    GameSpritePalette palettee8, palettefc;
    u8 unknown110, active_background;
    s8 selected_menu;
    s8 highlighted_menu;
    s8 menu_count;
    u8 unknown115;
    u8 available[4], displayed[4];
    u8 party_count;
    u8 unknown11f;
    u8 special_available, abilities[6];
    u8 unknown127[11];
    s16 label_origin_x, label_origin_y;
    u8 unknown136[0x182];
    int text_panel_active;
    void *renderer;
    int equipment_active;
    s8 selected[4];
    s8 equipment_comparison[5], equipment_frames[3], equipment_switching;
    u8 unknown2d1[3];
    int equipment_heading_x, equipment_heading_y;
    union {
        u8 unknown2dc[4];
        struct {
            s8 equipment_highlight_state;
            union { u8 unknown2dd, hp_warning_mode; };
            u16 equipment_highlight_item;
        };
    };
    GameSpriteAllocation main_allocation, sub_allocation;
    void *sprites[48];
    GamePaletteEffectController *palette_controller;
    GamePaletteEffectEntry palette_entries[129];
    u8 unknown_de8[90600 - 0xde8];
};
typedef char PauseSceneWorkSize[sizeof(PauseSceneWork) == 90600 ? 1 : -1];
/* Native alias of WORK + 0x200; item is the halfword at WORK + 0x2DE. */
struct HighlightItemView { u8 unknown_00[222]; u16 item; };
typedef char HighlightItemViewSize[sizeof(HighlightItemView) == 224 ? 1 : -1];

struct PauseSavedState {
    u8 unknown[1370];
    s16 tutorial;
};
typedef char PauseSavedStateSizeCheck[sizeof(PauseSavedState) == 1372 ? 1 : -1];
extern "C" {
extern GameTaskVTable data_ov007_0208d9b8, data_ov007_0208d9a4;
extern PauseSceneTask *data_ov007_0208e1e0;
extern void *data_ov007_0208e1e4;
extern u8 *data_ov007_020a6b90;
extern PauseSceneWork data_ov007_020905f0;
extern u8 data_ov007_0208e1e8[], data_ov007_0208e02c[];
extern GameRumblePattern data_ov007_0208d738[];
extern u8 data_0205a00c;
extern void *SceneManager_LoadResources(void *);
extern void SceneManager_Shutdown(void *), SceneController_Create(u8 *), SceneTask_LoadArchiveMember(u8 *);
extern void SceneScript_LoadPrimaryArchive(u8 *, int, u16);
extern void func_ov007_02081664(int), func_ov007_0208a918(void);
extern void PauseScene_ChooseEntryTask(PauseMenuElement *);
extern void *PauseParty_Init(void *storage, void *fonts), *PauseParty_Destroy(void *storage);
extern void func_ov005_02068c54(void *);

extern void MI_StopDma(int);
#define WORK data_ov007_020905f0
#define SAVED_TUTORIAL (((PauseSavedState *)gSaveData)->tutorial)
static inline void DeleteSceneManager(void *p)
{
    if (p) {
        SceneManager_Shutdown(p);
        GameHeap_Delete(p);
    }
}
static inline void DeleteParty(void *p)
{
    if (p) {
        PauseParty_Destroy(p);
        GameHeap_Delete(p);
    }
}
static inline void DeletePausePanel(PausePanel *p)
{
    if (p)
        p->deleteInstance();
}
extern void func_ov005_02066128(int), func_ov005_0206781c(int, int);
extern void func_ov005_02066638(int), func_ov005_020690f0(int), func_ov005_02069b54(void),
    func_ov005_02068ca8(int), func_ov005_02069674(int), func_ov005_02069998(int);
extern Overlay5Archive *func_ov005_02067584(Overlay5Archive *, int, int, void *, int);
extern void func_ov005_020671e4(Overlay5Archive *), func_ov005_02067424(Overlay5Archive *, u8);
extern void func_ov005_020695cc(void *);
extern void func_02009078(int, void *, void *), func_02009058(int);
extern void func_ov005_02065f10(void), func_ov005_02069928(void), func_ov005_02069660(void),
    func_ov005_02068c94(void), func_ov005_020699f0(void), func_ov005_020690dc(void),
    func_ov005_02066624(void);
extern void func_02035fd0(int), func_0203613c(int), func_02035e04(int), func_02035ebc(int),
    func_020359c4(int), func_02035a40(int), func_020358ac(int), func_02035938(int);
extern void MI_CpuFill8(void *, u8, u32);

extern Overlay5DisplayResources data_ov005_0206a180;
extern u8 data_0206032c[];
#define ARCHIVE data_ov005_0206a180.archive
#define OAM_BUFFERS ((u8 *)data_ov005_0206a180.oam)
#define REG16(a) (*(vu16 *)(a))
#define REG32(a) (*(vu32 *)(a))
extern PauseMenuElement *func_ov005_0206659c(void (*)(PauseMenuElement *), int, int);
}
#endif
