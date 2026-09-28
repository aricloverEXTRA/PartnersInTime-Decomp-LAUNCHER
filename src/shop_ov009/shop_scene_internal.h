#ifndef PIT_SHOP_SCENE_INTERNAL_H
#define PIT_SHOP_SCENE_INTERNAL_H

/*
 * The shop scene's workspace: its panels, its lists and the state shared between
 * the units that draw them.
 */
extern "C" {
#include <game/shop_scene.h>
#include <game/shop_panel_slide.h>
#include <game/shop_list.h>
#include <game/shop_panels.h>
#include <game/heap.h>
#include <game/save_data.h>
#include <game/input.h>
#include <game/overlay005_display.h>
#include <game/overlay005_archive.h>
#include <game/sprite_output.h>
#include <nitro/gx_init.h>
}
class ShopPanel
{
  public:
    virtual void destroy();
    virtual void deleteInstance();
};
typedef struct ShopSceneWork {
    void *owned_00, *owned_04;
    u8 font_slots[80];
    void *owned_58[9];
    u8 background_dirty, unknown_7d;
    s16 wave_pending, wave_current, unknown_82;
    void *owned_84[2];
    u8 unknown_8c[2], available[4], displayed[4], party_count;
    u8 variant, animated_background, special_shop;
    u8 unknown_9a[4], initialized;
    u8 selected_category;
    union {
        u8 unknown_a0[0x8c0 - 0xa0];
        struct {
            u8 unknown_a0_byte, panel_zooming, panel_visible, unknown_a3;
            s32 panel_scale_y;
            u8 unknown_a8[7];
            s8 equipment_highlight_active;
            u8 unknown_b0[0x8c0 - 0xb0];
        };
    };
    u8 unknown_8c0, unknown_8c1, selected_member;
    u8 unknown_8c3[5];
    s32 panel_offset_y;
    void *sprites[60];
} ShopSceneWork;
typedef char ShopSceneWorkSizeCheck[sizeof(ShopSceneWork) == 2492 ? 1 : -1];

typedef struct ShopScrollState {
    u8 unknown[196];
    s16 sub, main;
} ShopScrollState;
typedef char ShopScrollStateSizeCheck[sizeof(ShopScrollState) == 200 ? 1 : -1];
extern "C" ShopScrollState data_ov009_0207f23c;

typedef struct ShopSavedState {
    u8 unknown[1370];
    u8 unused : 4, shop : 4;
    u8 selections[3], previous_selections[3];
} ShopSavedState;
typedef struct ShopMenuElement ShopMenuElement;
extern "C" {
extern GameTaskVTable data_ov009_0207e0b8, data_ov009_0207e0a4;
extern ShopSceneTask *data_ov009_0207ea38;
extern ShopSceneWork data_ov009_0207ea3c;
extern ShopPanel *data_ov009_0207ea20, *data_ov009_0207ea24, *data_ov009_0207ea34;
extern void *data_ov009_0207ea28, *data_ov009_0207ea2c, *data_ov009_0207ea30;
extern u8 data_ov005_0206a180[], data_0206032c[];
#define ARCHIVE (*(Overlay5Archive **)(data_ov005_0206a180 + 44))
#define OAM_BUFFERS (*(u8 **)(data_ov005_0206a180 + 40))
#define REG16(a) (*(vu16 *)(a))
#define REG32(a) (*(vu32 *)(a))
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
extern void ShopScene_LoadResources(ShopSceneTask *);
extern ShopMenuElement *func_ov005_0206659c(void (*)(ShopMenuElement *), int, int);
extern void func_ov009_0206abd0(ShopMenuElement *), func_ov009_0206ca78(ShopMenuElement *);

extern u32 OS_DisableIrqMask(u32), OS_EnableIrqMask(u32);
extern void OS_SetIrqFunction(u32, void (*)(void));
extern int GX_HBlankIntr(int);
extern void func_ov005_020679fc(int, int, int, int), func_ov005_020679a4(int, int, int),
    func_ov005_0206786c(int, int, int, int, int, int);
extern void *func_ov009_02074b80(void *, void *);
extern void func_ov009_02074b7c(void *);
static inline void DeletePanel(ShopPanel *panel)
{
    if (panel)
        panel->deleteInstance();
}
static inline void DeleteShopDigitRenderer(void *p)
{
    if (p) {
        func_ov009_02074b7c(p);
        GameHeap_Delete(p);
    }
}
static inline void DeleteShopWidget(void *p)
{
    if (p) {
        ShopStockPanel_Destroy((ShopStockPanel *)p);
        GameHeap_Delete(p);
    }
}
static inline void DeleteShopList(void *p)
{
    if (p) {
        ShopList_Destroy((ShopList *)p);
        GameHeap_Delete(p);
    }
}
}
#endif
