/*
 * Shop scene lifecycle (overlay 9, 0x0206EC50-0x0207010C).
 *
 * Construction, per-frame graphics upload and teardown of the shop scene. Init
 * allocates the scene and its IRQ task, the IRQ task copies the prepared frame
 * into VRAM during the blanking period, and Destroy/Delete unwind that in the
 * order the task lists require.
 */

#include "shop_scene_internal.h"

extern "C" {
GameIrqTask *ShopSceneIrq_Init(GameIrqTask *task, u32 priority, u32 unused, void *parent)
{
    GameIrqTask_Init(task, priority, unused, parent);
    task->vtable = &data_ov009_0207e0a4;
    return task;
}
}

extern "C" {
extern int func_ov005_02065f00(void);
extern void func_ov005_0206651c(int, int), func_ov005_0206642c(int, int), func_ov005_02068f84(int),
    func_ov005_02065eec(void);
extern void MIi_CpuCopyFast(const void *, void *, u32);

#define WORK data_ov009_0207ea3c
#define SCROLL data_ov009_0207f23c
void ShopScene_UploadFrame(void)
{
    if (!func_ov005_02065f00())
        return;
    func_ov005_0206651c(11, 11);
    func_ov005_0206642c(11, 11);
    SCROLL.sub = -*(s32 *)((u8 *)&WORK + 0x8c8) / 4096;
    SCROLL.main = *(s32 *)((u8 *)&WORK + 0x8c8) / 4096;
    if (SCROLL.sub < 0)
        func_ov005_0206786c(1, 0, 0, 0, 255, SCROLL.sub + 192);
    else
        func_ov005_0206786c(1, 0, 0, SCROLL.sub, 255, 192);
    if (SCROLL.main < 0)
        func_ov005_0206786c(0, 0, 0, 0, 255, SCROLL.main + 192);
    else
        func_ov005_0206786c(0, 0, 0, SCROLL.main, 255, 192);
    if (!WORK.special_shop) {
        if (WORK.animated_background) {
            if (WORK.background_dirty) {
                void *destination = Overlay5DisplayBg_GetScreenVram16K(DISPLAY_ENGINE_SUB, 2);
                MIi_CpuCopyFast(WORK.owned_58[7], destination, 49152);
                WORK.background_dirty = 0;
            }
            WORK.wave_current = WORK.wave_pending;
        }
        Overlay5DisplayBg_SetVerticalOffset(DISPLAY_ENGINE_SUB, 1, -SCROLL.sub);
        Overlay5DisplayBg_SetAffine(DISPLAY_ENGINE_SUB, 2, 4096, 4096, 0, 0, 0, 0, -SCROLL.sub);
        Overlay5DisplayBg_SetAffine(DISPLAY_ENGINE_SUB, 3, 4096, 4096, 0, 0, 0, 0, -SCROLL.sub);
        Overlay5DisplayBg_SetVerticalOffset(DISPLAY_ENGINE_MAIN, 0, -SCROLL.main);
        Overlay5DisplayBg_SetVerticalOffset(DISPLAY_ENGINE_MAIN, 1, -SCROLL.main);
        Overlay5DisplayBg_SetAffine(DISPLAY_ENGINE_MAIN, 3, 4096, 4096, 0, 0, 0, 0, -SCROLL.main);
    } else {
        Overlay5DisplayBg_SetVerticalOffset(DISPLAY_ENGINE_SUB, 0, -SCROLL.sub);
        Overlay5DisplayBg_SetVerticalOffset(DISPLAY_ENGINE_SUB, 1, -SCROLL.sub);
        Overlay5DisplayBg_SetAffine(DISPLAY_ENGINE_SUB, 2, 4096, 4096, 0, 0, 0, 0, -SCROLL.sub);
        Overlay5DisplayBg_SetAffine(DISPLAY_ENGINE_SUB, 3, 4096, 4096, 0, 0, 0, 0, -SCROLL.sub);
        Overlay5DisplayBg_SetVerticalOffset(DISPLAY_ENGINE_MAIN, 0, -SCROLL.main);
        Overlay5DisplayBg_SetVerticalOffset(DISPLAY_ENGINE_MAIN, 1, -SCROLL.main);
        Overlay5DisplayBg_SetAffine(DISPLAY_ENGINE_MAIN, 3, 4096, 4096, 0, 0, 0, 0, -SCROLL.main);
    }
    func_ov005_02068f84(0);
    func_ov005_02068f84(1);
    GameSpritePalette_UploadScreen(0);
    GameSpritePalette_UploadScreen(1);
    GameDisplay_CopyOam(3);
    GameDisplay_ClearOam();
    func_ov005_02065eec();
}
}

extern "C" {
ShopSceneTask *ShopScene_Init(ShopSceneTask *task, u32 priority, u32 unused)
{
    Overlay5Archive *archive;
    GameIrqTask *irq;
    void *object;
    int i;
    GameTask_Init(&task->base, priority, unused, 0);
    task->base.vtable = &data_ov009_0207e0b8;
    data_ov009_0207ea38 = task;
    func_ov005_02066128(1);
    data_0206032c[8] = 12;
    data_0206032c[9] = 3;
    Overlay5Display_ResetEngine(DISPLAY_ENGINE_MAIN);
    Overlay5Display_ResetEngine(DISPLAY_ENGINE_SUB);
    REG16(0x04000304) &= ~0x8000;
    func_ov005_0206781c(2, -16);
    func_02035fd0(1);
    func_0203613c(2);
    func_02035e04(32);
    func_02035ebc(64);
    func_020359c4(8);
    func_02035a40(4);
    func_020358ac(256);
    func_02035938(128);
    GX_SetGraphicsMode(1, 5, 0);
    GXS_SetGraphicsMode(5);
    Overlay5Display_SetObjTileMappingMode(DISPLAY_ENGINE_MAIN, DISPLAY_OBJ_TILE_MAPPING_1D_128K);
    Overlay5Display_SetObjTileMappingMode(DISPLAY_ENGINE_SUB, DISPLAY_OBJ_TILE_MAPPING_1D_128K);
    Overlay5DisplayBg_ClearControl(DISPLAY_ENGINE_MAIN, 0);
    Overlay5DisplayBg_SetScreenSize(DISPLAY_ENGINE_MAIN, 0, 0);
    Overlay5DisplayBg_SetColorMode(DISPLAY_ENGINE_MAIN, 0, 0);
    Overlay5DisplayBg_SetScreenBase(DISPLAY_ENGINE_MAIN, 0, 0);
    Overlay5DisplayBg_SetCharacterBase(DISPLAY_ENGINE_MAIN, 0, 1);
    Overlay5DisplayBg_SetMosaic(DISPLAY_ENGINE_MAIN, 0, 0);
    Overlay5DisplayBg_SetPriority(DISPLAY_ENGINE_MAIN, 0, 0);
    Overlay5DisplayBg_ClearControl(DISPLAY_ENGINE_MAIN, 1);
    Overlay5DisplayBg_SetScreenSize(DISPLAY_ENGINE_MAIN, 1, 0);
    Overlay5DisplayBg_SetColorMode(DISPLAY_ENGINE_MAIN, 1, 1);
    Overlay5DisplayBg_SetScreenBase(DISPLAY_ENGINE_MAIN, 1, 1);
    Overlay5DisplayBg_SetCharacterBase(DISPLAY_ENGINE_MAIN, 1, 3);
    Overlay5DisplayBg_SetMosaic(DISPLAY_ENGINE_MAIN, 1, 0);
    Overlay5DisplayBg_SetAreaOverflow(DISPLAY_ENGINE_MAIN, 1, 1);
    Overlay5DisplayBg_SetPriority(DISPLAY_ENGINE_MAIN, 1, 2);
    Overlay5DisplayBg_ClearControl(DISPLAY_ENGINE_MAIN, 2);
    Overlay5DisplayBg_SetScreenSize(DISPLAY_ENGINE_MAIN, 2, 1);
    Overlay5DisplayBg_SetModeBits(DISPLAY_ENGINE_MAIN, 2, 0);
    Overlay5DisplayBg_SetScreenBase(DISPLAY_ENGINE_MAIN, 2, 2);
    Overlay5DisplayBg_SetCharacterBase(DISPLAY_ENGINE_MAIN, 2, 3);
    Overlay5DisplayBg_SetMosaic(DISPLAY_ENGINE_MAIN, 2, 0);
    Overlay5DisplayBg_SetAreaOverflow(DISPLAY_ENGINE_MAIN, 2, 1);
    Overlay5DisplayBg_SetOverflow(DISPLAY_ENGINE_MAIN, 2, 0);
    Overlay5DisplayBg_SetPriority(DISPLAY_ENGINE_MAIN, 2, 1);
    Overlay5DisplayBg_ClearControl(DISPLAY_ENGINE_MAIN, 3);
    Overlay5DisplayBg_SetScreenSize(DISPLAY_ENGINE_MAIN, 3, 1);
    Overlay5DisplayBg_SetModeBits(DISPLAY_ENGINE_MAIN, 3, 0);
    Overlay5DisplayBg_SetScreenBase(DISPLAY_ENGINE_MAIN, 3, 3);
    Overlay5DisplayBg_SetCharacterBase(DISPLAY_ENGINE_MAIN, 3, 4);
    Overlay5DisplayBg_SetMosaic(DISPLAY_ENGINE_MAIN, 3, 0);
    Overlay5DisplayBg_SetAreaOverflow(DISPLAY_ENGINE_MAIN, 3, 1);
    Overlay5DisplayBg_SetPriority(DISPLAY_ENGINE_MAIN, 3, 3);
    Overlay5DisplayBg_ClearControl(DISPLAY_ENGINE_SUB, 0);
    Overlay5DisplayBg_SetScreenSize(DISPLAY_ENGINE_SUB, 0, 0);
    Overlay5DisplayBg_SetColorMode(DISPLAY_ENGINE_SUB, 0, 0);
    Overlay5DisplayBg_SetScreenBase(DISPLAY_ENGINE_SUB, 0, 0);
    Overlay5DisplayBg_SetCharacterBase(DISPLAY_ENGINE_SUB, 0, 1);
    Overlay5DisplayBg_SetMosaic(DISPLAY_ENGINE_SUB, 0, 0);
    Overlay5DisplayBg_SetPriority(DISPLAY_ENGINE_SUB, 0, 1);
    Overlay5DisplayBg_ClearControl(DISPLAY_ENGINE_SUB, 1);
    Overlay5DisplayBg_SetScreenSize(DISPLAY_ENGINE_SUB, 1, 0);
    Overlay5DisplayBg_SetColorMode(DISPLAY_ENGINE_SUB, 1, 1);
    Overlay5DisplayBg_SetScreenBase(DISPLAY_ENGINE_SUB, 1, 1);
    Overlay5DisplayBg_SetCharacterBase(DISPLAY_ENGINE_SUB, 1, 3);
    Overlay5DisplayBg_SetMosaic(DISPLAY_ENGINE_SUB, 1, 0);
    Overlay5DisplayBg_SetAreaOverflow(DISPLAY_ENGINE_SUB, 1, 1);
    Overlay5DisplayBg_SetPriority(DISPLAY_ENGINE_SUB, 1, 0);
    Overlay5DisplayBg_ClearControl(DISPLAY_ENGINE_SUB, 2);
    Overlay5DisplayBg_SetScreenSize(DISPLAY_ENGINE_SUB, 2, 1);
    Overlay5DisplayBg_SetModeBits(DISPLAY_ENGINE_SUB, 2, 128);
    Overlay5DisplayBg_SetScreenBase(DISPLAY_ENGINE_SUB, 2, 5);
    Overlay5DisplayBg_SetMosaic(DISPLAY_ENGINE_SUB, 2, 0);
    Overlay5DisplayBg_SetAreaOverflow(DISPLAY_ENGINE_SUB, 2, 1);
    Overlay5DisplayBg_SetPriority(DISPLAY_ENGINE_SUB, 2, 2);
    Overlay5DisplayBg_ClearControl(DISPLAY_ENGINE_SUB, 3);
    Overlay5DisplayBg_SetScreenSize(DISPLAY_ENGINE_SUB, 3, 1);
    Overlay5DisplayBg_SetModeBits(DISPLAY_ENGINE_SUB, 3, 0);
    Overlay5DisplayBg_SetScreenBase(DISPLAY_ENGINE_SUB, 3, 3);
    Overlay5DisplayBg_SetCharacterBase(DISPLAY_ENGINE_SUB, 3, 4);
    Overlay5DisplayBg_SetMosaic(DISPLAY_ENGINE_SUB, 3, 0);
    Overlay5DisplayBg_SetAreaOverflow(DISPLAY_ENGINE_SUB, 3, 1);
    Overlay5DisplayBg_SetPriority(DISPLAY_ENGINE_SUB, 3, 3);
    func_ov005_02066638(288);
    func_ov005_020690f0(256);
    func_ov005_02069b54();
    func_ov005_02068ca8(96);
    func_ov005_02069674(288);
    func_ov005_02069998(6);
    archive = (Overlay5Archive *)GameHeap_New(sizeof(Overlay5Archive), 0, 0, 1);
    if (archive)
        archive = func_ov005_02067584(archive, 10, 0, 0, -1);
    ARCHIVE = archive;
    DisplayVram_ClearEngineAssignments(0);
    DisplayVram_ClearEngineAssignments(1);
    OAM_BUFFERS = (u8 *)GameHeap_NewArray(17408, 0, 0, 0);
    func_02009078(0, OAM_BUFFERS, OAM_BUFFERS + 8192);
    func_02009078(1, OAM_BUFFERS + 8704, OAM_BUFFERS + 16896);
    irq = (GameIrqTask *)GameHeap_New(40, 0, 0, 0);
    if (irq)
        irq = ShopSceneIrq_Init(irq, 8, 0, task);
    task->irq = irq;
    GameDisplay_SetBackgroundColor(0, 0);
    REG16(0x04000050) = 0;
    REG16(0x04001050) = 0;
    REG32(0x04000000) = (REG32(0x04000000) & ~0x1F00) | 0x1B00;
    REG32(0x04001000) = (REG32(0x04001000) & ~0x1F00) | 0x1800;
    GX_DispOn();
    REG32(0x04001000) |= 0x10000;
    MI_CpuFill8(&data_ov009_0207ea3c, 0, sizeof(data_ov009_0207ea3c));
    ShopScene_LoadResources(task);
    if (!((ShopSavedState *)gSaveData)->shop) {
        /* The original loop writes the same two bytes three times. */
        for (i = 0; i < 3; ++i) {
            ((ShopSavedState *)gSaveData)->selections[0] = 0;
            ((ShopSavedState *)gSaveData)->previous_selections[0] = 0;
        }
    }
    object = GameHeap_New(2096, 0, 0, 0);
    if (object)
        object = func_ov009_02074b80(object, data_ov009_0207ea3c.font_slots);
    data_ov009_0207ea30 = object;
    object = GameHeap_New(936, 0, 0, 0);
    if (object)
        object = ShopBuyPanel_Init((ShopBuyPanel *)object);
    data_ov009_0207ea34 = (ShopPanel *)object;
    object = GameHeap_New(224, 0, 0, 0);
    if (object)
        object = ShopStockPanel_Init((ShopStockPanel *)object);
    data_ov009_0207ea28 = object;
    object = GameHeap_New(848, 0, 0, 0);
    if (object)
        object = ShopInventoryPanel_Init((ShopInventoryPanel *)object);
    data_ov009_0207ea24 = (ShopPanel *)object;
    object = GameHeap_New(656, 0, 0, 0);
    if (object)
        object = ShopList_Init((ShopList *)object);
    data_ov009_0207ea2c = object;
    if (!data_ov009_0207ea3c.special_shop) {
        if (((ShopSavedState *)gSaveData)->shop)
            data_ov009_0207ea3c.selected_category = ((ShopSavedState *)gSaveData)->shop - 1;
        data_ov009_0207ea20 = data_ov009_0207ea34;
        func_ov005_0206659c(func_ov009_0206ca78, 0, 1);
        task->phase = 0;
    } else {
        data_ov009_0207ea20 = data_ov009_0207ea24;
        func_ov005_0206659c(func_ov009_0206abd0, 0, 1);
        task->phase = 0;
    }
    if (((ShopSavedState *)gSaveData)->shop)
        ShopPanelSlide_Begin(1);
    func_ov005_020679fc(0, 0, 31, 1);
    func_ov005_020679fc(1, 0, 31, 1);
    func_ov005_020679a4(0, 0, 1);
    func_ov005_020679a4(1, 0, 1);
    func_ov005_0206786c(0, 0, 0, 0, 255, 192);
    func_ov005_0206786c(1, 0, 0, 0, 255, 192);
    Overlay5Display_SetVisibleWindows(DISPLAY_ENGINE_MAIN, 1);
    Overlay5Display_SetVisibleWindows(DISPLAY_ENGINE_SUB, 1);
    GameIrqTask_Enable(task->irq);
    if (!data_ov009_0207ea3c.special_shop && data_ov009_0207ea3c.animated_background) {
        OS_DisableIrqMask(2);
        OS_SetIrqFunction(2, ShopScene_UpdateHBlank);
        OS_EnableIrqMask(2);
        GX_HBlankIntr(1);
    }
    return task;
}

ShopSceneTask *ShopScene_Destroy(ShopSceneTask *task)
{
    int i;
    task->base.vtable = &data_ov009_0207e0b8;
    func_ov005_020671e4(ARCHIVE);
    if (task->irq) {
        GameIrqTask_DeleteSafe(task->irq);
        task->irq = 0;
    }
    if (!data_ov009_0207ea3c.special_shop && data_ov009_0207ea3c.animated_background) {
        OS_DisableIrqMask(2);
        GX_HBlankIntr(0);
        REG32(0x04000000) &= ~0x800000;
    }
    for (i = 0; i < 60; ++i) {
        if (data_ov009_0207ea3c.sprites[i]) {
            func_ov005_020695cc(data_ov009_0207ea3c.sprites[i]);
            data_ov009_0207ea3c.sprites[i] = 0;
        }
    }
    for (i = 0; i < 3; ++i)
        func_ov005_02067424(ARCHIVE, (u8)i);
    if (data_ov009_0207ea3c.owned_84[0]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_84[0]);
        data_ov009_0207ea3c.owned_84[0] = 0;
    }
    if (data_ov009_0207ea3c.owned_84[1]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_84[1]);
        data_ov009_0207ea3c.owned_84[1] = 0;
    }
    if (data_ov009_0207ea3c.owned_58[5]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[5]);
        data_ov009_0207ea3c.owned_58[5] = 0;
    }
    if (data_ov009_0207ea3c.owned_58[6]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[6]);
        data_ov009_0207ea3c.owned_58[6] = 0;
    }
    if (data_ov009_0207ea3c.owned_58[8]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[8]);
        data_ov009_0207ea3c.owned_58[8] = 0;
    }
    if (data_ov009_0207ea3c.owned_58[7]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[7]);
        data_ov009_0207ea3c.owned_58[7] = 0;
    }
    if (data_ov009_0207ea3c.owned_58[4]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[4]);
        data_ov009_0207ea3c.owned_58[4] = 0;
    }
    if (data_ov009_0207ea3c.owned_58[3]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[3]);
        data_ov009_0207ea3c.owned_58[3] = 0;
    }
    if (data_ov009_0207ea3c.owned_58[2]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[2]);
        data_ov009_0207ea3c.owned_58[2] = 0;
    }
    if (data_ov009_0207ea3c.owned_58[1]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[1]);
        data_ov009_0207ea3c.owned_58[1] = 0;
    }
    {
        void *object = data_ov009_0207ea30;
        if (object) {
            DeleteShopDigitRenderer(object);
            data_ov009_0207ea30 = 0;
        }
    }
    if (data_ov009_0207ea34) {
        DeletePanel(data_ov009_0207ea34);
        data_ov009_0207ea34 = 0;
    }
    {
        void *object = data_ov009_0207ea28;
        if (object) {
            DeleteShopWidget(object);
            data_ov009_0207ea28 = 0;
        }
    }
    if (data_ov009_0207ea24) {
        DeletePanel(data_ov009_0207ea24);
        data_ov009_0207ea24 = 0;
    }
    {
        void *object = data_ov009_0207ea2c;
        if (object) {
            DeleteShopList(object);
            data_ov009_0207ea2c = 0;
        }
    }
    if (data_ov009_0207ea3c.owned_58[0]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[0]);
        data_ov009_0207ea3c.owned_58[0] = 0;
    }
    if (data_ov009_0207ea3c.owned_00) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_00);
        data_ov009_0207ea3c.owned_00 = 0;
    }
    if (data_ov009_0207ea3c.owned_04) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_04);
        data_ov009_0207ea3c.owned_04 = 0;
    }
    func_ov005_02065f10();
    func_02009058(0);
    func_02009058(1);
    func_ov005_02069928();
    func_ov005_02069660();
    func_ov005_02068c94();
    func_ov005_020699f0();
    func_ov005_020690dc();
    func_ov005_02066624();
    Overlay5Display_ResetEngine(DISPLAY_ENGINE_MAIN);
    Overlay5Display_ResetEngine(DISPLAY_ENGINE_SUB);
    func_ov005_0206781c(2, 0);
    data_ov009_0207ea38 = 0;
    GameTask_DestroyBase(&task->base);
    return task;
}

ShopSceneTask *ShopScene_Delete(ShopSceneTask *task)
{
    int i;
    task->base.vtable = &data_ov009_0207e0b8;
    func_ov005_020671e4(ARCHIVE);
    if (task->irq) {
        GameIrqTask_DeleteSafe(task->irq);
        task->irq = 0;
    }
    if (!data_ov009_0207ea3c.special_shop && data_ov009_0207ea3c.animated_background) {
        OS_DisableIrqMask(2);
        GX_HBlankIntr(0);
        REG32(0x04000000) &= ~0x800000;
    }
    for (i = 0; i < 60; ++i) {
        if (data_ov009_0207ea3c.sprites[i]) {
            func_ov005_020695cc(data_ov009_0207ea3c.sprites[i]);
            data_ov009_0207ea3c.sprites[i] = 0;
        }
    }
    for (i = 0; i < 3; ++i)
        func_ov005_02067424(ARCHIVE, (u8)i);
    if (data_ov009_0207ea3c.owned_84[0]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_84[0]);
        data_ov009_0207ea3c.owned_84[0] = 0;
    }
    if (data_ov009_0207ea3c.owned_84[1]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_84[1]);
        data_ov009_0207ea3c.owned_84[1] = 0;
    }
    if (data_ov009_0207ea3c.owned_58[5]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[5]);
        data_ov009_0207ea3c.owned_58[5] = 0;
    }
    if (data_ov009_0207ea3c.owned_58[6]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[6]);
        data_ov009_0207ea3c.owned_58[6] = 0;
    }
    if (data_ov009_0207ea3c.owned_58[8]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[8]);
        data_ov009_0207ea3c.owned_58[8] = 0;
    }
    if (data_ov009_0207ea3c.owned_58[7]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[7]);
        data_ov009_0207ea3c.owned_58[7] = 0;
    }
    if (data_ov009_0207ea3c.owned_58[4]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[4]);
        data_ov009_0207ea3c.owned_58[4] = 0;
    }
    if (data_ov009_0207ea3c.owned_58[3]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[3]);
        data_ov009_0207ea3c.owned_58[3] = 0;
    }
    if (data_ov009_0207ea3c.owned_58[2]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[2]);
        data_ov009_0207ea3c.owned_58[2] = 0;
    }
    if (data_ov009_0207ea3c.owned_58[1]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[1]);
        data_ov009_0207ea3c.owned_58[1] = 0;
    }
    {
        void *object = data_ov009_0207ea30;
        if (object) {
            DeleteShopDigitRenderer(object);
            data_ov009_0207ea30 = 0;
        }
    }
    if (data_ov009_0207ea34) {
        DeletePanel(data_ov009_0207ea34);
        data_ov009_0207ea34 = 0;
    }
    {
        void *object = data_ov009_0207ea28;
        if (object) {
            DeleteShopWidget(object);
            data_ov009_0207ea28 = 0;
        }
    }
    if (data_ov009_0207ea24) {
        DeletePanel(data_ov009_0207ea24);
        data_ov009_0207ea24 = 0;
    }
    {
        void *object = data_ov009_0207ea2c;
        if (object) {
            DeleteShopList(object);
            data_ov009_0207ea2c = 0;
        }
    }
    if (data_ov009_0207ea3c.owned_58[0]) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_58[0]);
        data_ov009_0207ea3c.owned_58[0] = 0;
    }
    if (data_ov009_0207ea3c.owned_00) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_00);
        data_ov009_0207ea3c.owned_00 = 0;
    }
    if (data_ov009_0207ea3c.owned_04) {
        GameHeap_DeleteArray(data_ov009_0207ea3c.owned_04);
        data_ov009_0207ea3c.owned_04 = 0;
    }
    func_ov005_02065f10();
    func_02009058(0);
    func_02009058(1);
    func_ov005_02069928();
    func_ov005_02069660();
    func_ov005_02068c94();
    func_ov005_020699f0();
    func_ov005_020690dc();
    func_ov005_02066624();
    Overlay5Display_ResetEngine(DISPLAY_ENGINE_MAIN);
    Overlay5Display_ResetEngine(DISPLAY_ENGINE_SUB);
    func_ov005_0206781c(2, 0);
    data_ov009_0207ea38 = 0;
    GameTask_DestroyBase(&task->base);
    GameHeap_Delete(task);
    return task;
}
}
