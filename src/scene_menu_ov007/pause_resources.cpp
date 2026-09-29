extern "C" {

/*
 * Pause menu resources (overlay 7, 0x02070A2C-0x02071F80).
 *
 * Loads the graphics the pause menu draws from: the archive entries for its
 * backgrounds, panels and icons, the background extended palettes it queues for
 * upload, and the decompression of the entries that are stored compressed. Also
 * holds the scene's exit and transition-progress entry points, which is where
 * the menu hands control back.
 */

#include <nitro.h>
}
#include "pause_scene_internal.h"
extern "C" {
#include <game/script_vm.h>
}
struct ResourceEntry {
    u32 index, mode;
};
struct ResourceEntries {
    ResourceEntry entries[13];
};
struct SixValues {
    u32 entries[6];
};
struct AssetRow {
    u16 image, palette;
    u8 unknown4[3];
    union {
        u8 flags;
        struct {
            u8 localized : 1, unknown1 : 7;
        } bits;
    };
};
struct AssetTable {
    u32 count, unknown4;
    AssetRow rows[1];
};
typedef char ResourceEntriesSizeCheck[sizeof(ResourceEntries) == 104 ? 1 : -1];
typedef char SixValuesSizeCheck[sizeof(SixValues) == 24 ? 1 : -1];
typedef char AssetRowSizeCheck[sizeof(AssetRow) == 8 ? 1 : -1];
typedef char AssetTableSizeCheck[sizeof(AssetTable) == 16 ? 1 : -1];
extern "C" {

extern ResourceEntries data_ov007_0208da58;
extern SixValues data_ov007_0208d9dc, data_ov007_0208d9f4;
extern const void *data_ov007_0208d954[];
extern u8 data_ov007_0208dac0[];
extern void *data_ov007_0208e1e4;
extern void *func_ov005_0206964c(void);
extern void func_ov005_020698c8(u32, void *);
extern void *Overlay5Archive_ReadEntry(Overlay5Archive *, int, u16, u32 *, int);
extern void *func_ov005_02066f78(Overlay5Archive *, int, u16, u32 *, int);
extern void func_ov005_02067468(Overlay5Archive *, u8, const void *, int);
extern void func_ov005_02067424(Overlay5Archive *, u8);
extern Overlay5Asset *func_ov005_02066a7c(Overlay5Archive *, u16, u16, const AssetTable *, u8, int);
extern void *func_ov005_0206687c(const char *, u32 *, int);
#include <game/menu_number.h>
extern void func_02007ebc(GameSpritePalette *, int, int, int, int, int, int, int, int, int);
extern void MIi_CpuClear16(u16, void *, u32), MIi_CpuClearFast(u32, void *, u32),
    func_0203b7a0(u32, void *, u32);
extern void MI_UncompressLZ16(const void *, void *);
extern void DC_FlushRange(const void *, u32);
#undef ARCHIVE
#define DISPLAY data_ov005_0206a180
#define ARCHIVE DISPLAY.archive
}

extern "C" {
static inline void Clear16(void *destination, u32 bytes)
{
    volatile u16 zero = 0;
    MIi_CpuClear16(zero, destination, bytes);
}
static inline void ClearFast(void *destination, u32 bytes)
{
    volatile u32 zero = 0;
    MIi_CpuClearFast(zero, destination, bytes);
}
static inline void Clear32(void *destination, u32 bytes)
{
    volatile u32 zero = 0;
    func_0203b7a0(zero, destination, bytes);
}
static inline void DeleteBuffer(void *buffer)
{
    if (buffer)
        GameHeap_DeleteArray(buffer);
}
static inline void LoadCharacters(DisplayEngine screen, int layer, int entry, int offset)
{
    void *buffer = Overlay5Archive_ReadEntry(ARCHIVE, 1, entry, 0, 0);
    Overlay5DisplayBg_LoadCharactersLZ(screen, layer, buffer, offset);
    DeleteBuffer(buffer);
}
static inline void LoadScreen(DisplayEngine screen, int layer, int entry, int offset)
{
    void *buffer = Overlay5Archive_ReadEntry(ARCHIVE, 1, entry, 0, 0);
    Overlay5DisplayBg_LoadScreenLZ(screen, layer, buffer, offset);
    DeleteBuffer(buffer);
}
static inline void LoadExtBgPalette(DisplayEngine screen, int entry, int offset, int clear_first)
{
    u8 *buffer = (u8 *)Overlay5Archive_ReadEntry(ARCHIVE, 1, entry, 0, 0);
    if (clear_first) {
        buffer[0] = 0;
        buffer[1] = 0;
    }
    DC_FlushRange(buffer, 512);
    Overlay5Display_BeginLoadBgExtPalette(screen);
    Overlay5Display_LoadBgExtPalette(screen, buffer, offset, 512);
    Overlay5Display_LoadBgExtPalette(screen, buffer, offset + 8192, 512);
    Overlay5Display_EndLoadBgExtPalette(screen);
    DeleteBuffer(buffer);
}
static inline void UploadObjPaletteBoth(void *buffer, int offset, u32 *size)
{
    DC_FlushRange(buffer, *size);
    Overlay5Display_BeginLoadObjExtPalette(DISPLAY_ENGINE_MAIN);
    Overlay5Display_LoadObjExtPalette(DISPLAY_ENGINE_MAIN, buffer, offset, *size);
    Overlay5Display_EndLoadObjExtPalette(DISPLAY_ENGINE_MAIN);
    Overlay5Display_BeginLoadObjExtPalette(DISPLAY_ENGINE_SUB);
    Overlay5Display_LoadObjExtPalette(DISPLAY_ENGINE_SUB, buffer, offset, *size);
    Overlay5Display_EndLoadObjExtPalette(DISPLAY_ENGINE_SUB);
}
void PauseScene_LoadResources(PauseSceneTask *)
{
    ResourceEntries resource_entries = data_ov007_0208da58;
    GamePaletteEffectController *controller = (GamePaletteEffectController *)GameHeap_New(12, 0, 0, 0);
    if (controller)
        controller = GamePaletteEffects_Init(controller, 0, WORK.palette_entries, 129);
    WORK.palette_controller = controller;
    WORK.initialized = 1;
    WORK.saved_party_state = *(s16 *)(gSaveData + 1368);
    for (int i = 0; i < 4; ++i)
        WORK.available[i] = 1;
    WORK.party_count = 4;
    if (VM_ReadVariable(8197, 0, 0)) {
        WORK.available[1] = 0;
        WORK.available[2] = 0;
        WORK.available[3] = 0;
        WORK.party_count = 1;
    }
    if (VM_ReadVariable(8198, 0, 0)) {
        WORK.available[2] = 0;
        WORK.available[3] = 0;
        WORK.party_count = 2;
    }
    if (VM_ReadVariable(8199, 0, 0)) {
        WORK.available[0] = 0;
        WORK.available[1] = 0;
        WORK.party_count = 2;
    }
    for (int i = 0; i < 4; ++i)
        WORK.displayed[i] = WORK.available[i];
    if (VM_ReadVariable(8219, 0, 0))
        for (int i = 0; i < 4; ++i)
            WORK.displayed[i] = 1;
    WORK.selected[0] = 0;
    for (int i = 0; i < 4; ++i) {
        if (WORK.available[i])
            break;
        WORK.selected[0] = i + 1;
    }
    WORK.selected[1] = WORK.selected[0];
    WORK.selected[2] = WORK.selected[0];
    WORK.selected[3] = WORK.selected[0];
    WORK.menu_count = 5;
    if (!VM_ReadVariable(8210, 0, 0))
        WORK.menu_count = 4;
    if (!VM_ReadVariable(8209, 0, 0))
        WORK.menu_count = 3;
    if (VM_ReadVariable(8218, 0, 0))
        WORK.special_available = 1;
    SixValues variables = data_ov007_0208d9dc;
    for (int i = 0; i < 6; ++i)
        if (VM_ReadVariable((u16)variables.entries[i], 0, 0))
            WORK.abilities[i] = 1;
    for (int i = 0; i < 48; ++i)
        WORK.sprites[i] = func_ov005_0206964c();
    WORK.number_scratch = GameHeap_NewArray(12288, 0, 0, 0);
    WORK.owned88 = GameHeap_NewArray(49152, 0, 0, 0);
    WORK.owned8c = GameHeap_NewArray(17920, 0, 0, 0);
    for (int i = 0; i < 2; ++i) {
        WORK.background_tiles[i] = GameHeap_NewArray(32768, 0, 0, 0);
        WORK.background_maps[i] = GameHeap_NewArray(2048, 0, 0, 0);
        WORK.secondary_background_maps[i] = GameHeap_NewArray(2048, 0, 0, 0);
        Clear16(WORK.background_maps[i], 2048);
        Clear16(WORK.secondary_background_maps[i], 2048);
    }
    WORK.number_glyph_pixels = GameHeap_NewArray(2048, 0, 0, 0);
    Clear32(WORK.owned88, 49152);
    WORK.background_dirty = 1;
    GameSpriteAllocation_Allocate(&DISPLAY.allocations[0], 0, 0, 220, 0, 65535, 1, 0);
    GameSpriteAllocation_Allocate(&DISPLAY.allocations[1], 1, 0, 210, 0, 65535, 1, 0);
    func_02007ebc(&DISPLAY.palettes[0], 0, 0, 16, 0, 2, 0, 0, 0, 65535);
    func_02007ebc(&DISPLAY.palettes[1], 1, 0, 16, 0, 4, 0, 0, 0, 65535);
    func_02007ebc(&DISPLAY.palettes[2], 0, 0, 0, 0, 16, 0, 0, 0, 65535);
    func_02007ebc(&DISPLAY.palettes[3], 1, 0, 0, 0, 16, 0, 0, 0, 65535);
    func_02007ebc(&WORK.palette98, 0, 0, 0, 0, 16, 0, 0, 0, 65534);
    func_02007ebc(&WORK.paletteac, 1, 0, 0, 0, 16, 0, 0, 0, 65534);
    func_02007ebc(&WORK.palettee8, 0, 0, 0, 0, 16, 0, 0, 0, 65533);
    func_02007ebc(&WORK.palettefc, 1, 0, 0, 0, 16, 0, 0, 0, 65533);
    ClearFast((void *)0x6000000, 16448);
    ClearFast((void *)0x6206800, 6144);
    Clear16((void *)0x6200000, 64);
    u32 size;
    func_ov005_02067468(ARCHIVE, 0, data_ov007_0208dac0, 1);
    {
        AssetTable *table = (AssetTable *)Overlay5Archive_ReadEntry(ARCHIVE, 0, 0, 0, 1);
        u16 row = 25;
        AssetRow *rows = table->rows, *palettes = &rows[table->count];
        if (rows[row].bits.localized)
            row += gSaveData[0x515];
        void *buffer = Overlay5Archive_ReadEntry(ARCHIVE, 0, palettes[rows[row].palette].image, &size, 0);
        UploadObjPaletteBoth(buffer, 1024, &size);
        DeleteBuffer(buffer);
        DeleteBuffer(table);
    }
    func_ov005_02067424(ARCHIVE, 0);
    func_ov005_02067468(ARCHIVE, 1, data_ov007_0208d954[1], 1);
    LoadCharacters(DISPLAY_ENGINE_MAIN, 3, 3, 0);
    {
        void *buffer;
        if (WORK.menu_count == 3)
            buffer = Overlay5Archive_ReadEntry(ARCHIVE, 1, 5, 0, 0);
        else if (WORK.menu_count == 4)
            buffer = Overlay5Archive_ReadEntry(ARCHIVE, 1, 6, 0, 0);
        else
            buffer = Overlay5Archive_ReadEntry(ARCHIVE, 1, 4, 0, 0);
        Overlay5DisplayBg_LoadScreenLZ(DISPLAY_ENGINE_MAIN, 3, buffer, 0);
        DeleteBuffer(buffer);
    }
    LoadExtBgPalette(DISPLAY_ENGINE_MAIN, 7, 0x4000, 1);
    {
        void *buffer = Overlay5Archive_ReadEntry(ARCHIVE, 1, 8, 0, 0);
        u8 *destination = (u8 *)Overlay5DisplayBg_GetCharacterVram(DISPLAY_ENGINE_MAIN, 3) + 0x10000;
        MI_UncompressLZ16(buffer, destination);
        DeleteBuffer(buffer);
    }
    {
        void *buffer;
        if (WORK.menu_count == 3)
            buffer = Overlay5Archive_ReadEntry(ARCHIVE, 1, 10, 0, 0);
        else if (WORK.menu_count == 4)
            buffer = Overlay5Archive_ReadEntry(ARCHIVE, 1, 11, 0, 0);
        else
            buffer = Overlay5Archive_ReadEntry(ARCHIVE, 1, 9, 0, 0);
        u8 *destination = (u8 *)Overlay5DisplayBg_GetScreenVram2K(DISPLAY_ENGINE_MAIN, 3) + 2048;
        MI_UncompressLZ16(buffer, destination);
        DeleteBuffer(buffer);
    }
    LoadExtBgPalette(DISPLAY_ENGINE_MAIN, 12, 0x4200, 0);
    LoadCharacters(DISPLAY_ENGINE_SUB, 3, 13, 0);
    LoadScreen(DISPLAY_ENGINE_SUB, 3, 14, 0);
    LoadExtBgPalette(DISPLAY_ENGINE_SUB, 15, 0x4000, 1);
    LoadCharacters(DISPLAY_ENGINE_SUB, 1, 16, 0x2000);
    {
        void *buffer = Overlay5Archive_ReadEntry(ARCHIVE, 1, 17, 0, 0);
        Overlay5DisplayBg_LoadScreenLZ(DISPLAY_ENGINE_SUB, 1, buffer, 0);
        u16 *screen = (u16 *)Overlay5DisplayBg_GetScreenVram2K(DISPLAY_ENGINE_SUB, 1);
        for (int i = 0; i < 768; ++i) {
            *screen += 0x5100;
            ++screen;
        }
        if (WORK.party_count == 1) {
            u16 *screen = (u16 *)Overlay5DisplayBg_GetScreenVram2K(DISPLAY_ENGINE_SUB, 1);
            for (int row = 0; row < 32; ++row)
                for (int col = 0; col < 16; ++col)
                    screen[32 * row + col] = 0;
        }
        DeleteBuffer(buffer);
    }
    {
        void *buffer = Overlay5Archive_ReadEntry(ARCHIVE, 1, 18, &size, 0);
        Overlay5Display_LoadBgPalette(DISPLAY_ENGINE_SUB, buffer, 160, size);
        DeleteBuffer(buffer);
    }
    LoadCharacters(DISPLAY_ENGINE_MAIN, 2, 0, 0);
    {
        void *buffer = Overlay5Archive_ReadEntry(ARCHIVE, 1, 1, 0, 0);
        Overlay5DisplayBg_LoadScreenLZ(DISPLAY_ENGINE_MAIN, 2, buffer, 0);
        u16 *screen = (u16 *)Overlay5DisplayBg_GetScreenVram2K(DISPLAY_ENGINE_MAIN, 2);
        for (int i = 0; i < 768; ++i) {
            *screen += 0x2000;
            ++screen;
        }
        DeleteBuffer(buffer);
    }
    {
        u8 *buffer = (u8 *)Overlay5Archive_ReadEntry(ARCHIVE, 1, 2, 0, 0);
        DC_FlushRange(buffer, 512);
        Overlay5Display_BeginLoadBgExtPalette(DISPLAY_ENGINE_MAIN);
        Overlay5Display_LoadBgExtPalette(DISPLAY_ENGINE_MAIN, buffer, 0x4400, 512);
        Overlay5Display_LoadBgExtPalette(DISPLAY_ENGINE_MAIN, buffer, 0x6400, 512);
        Overlay5Display_EndLoadBgExtPalette(DISPLAY_ENGINE_MAIN);
        if (buffer) {
            GameHeap_DeleteArray(buffer);
            buffer = 0;
        }
        WORK.owned94 = func_ov005_02066f78(ARCHIVE, 1, 56, 0, 0);
        Overlay5Display_LoadObjTiles(DISPLAY_ENGINE_MAIN, WORK.owned94, DISPLAY.allocations[0].offset, 12288);
        Overlay5Display_LoadObjTiles(DISPLAY_ENGINE_SUB, WORK.owned94, DISPLAY.allocations[1].offset, 12288);
        DeleteBuffer(buffer);
    }
    {
        void *buffer = Overlay5Archive_ReadEntry(ARCHIVE, 1, 57, &size, 0);
        UploadObjPaletteBoth(buffer, 512, &size);
        DeleteBuffer(buffer);
    }
    if (gSaveData[0x515] != 3 && gSaveData[0x515] != 5)
        WORK.number_glyph_tiles = func_ov005_02066f78(ARCHIVE, 1, 67, 0, 0);
    else
        WORK.number_glyph_tiles = func_ov005_02066f78(ARCHIVE, 1, 68, 0, 0);
    MenuNumber_PrepareGlyphs();
    {
        void *buffer = Overlay5Archive_ReadEntry(ARCHIVE, 1, 65, 0, 0);
        Overlay5Display_LoadObjTilesLZ(DISPLAY_ENGINE_SUB, buffer, DISPLAY.allocations[1].offset + 4736);
        DeleteBuffer(buffer);
    }
    {
        void *buffer = Overlay5Archive_ReadEntry(ARCHIVE, 1, 66, 0, 0);
        Overlay5Display_LoadObjPalette(DISPLAY_ENGINE_SUB, buffer, 32 * ((DISPLAY.palettes[1].bank + 2) & 15),
                                       64);
        DeleteBuffer(buffer);
    }
    WORK.ownedc0 = func_ov005_02066f78(ARCHIVE, 1, 69, 0, 0);
    {
        u8 *buffer = (u8 *)Overlay5Archive_ReadEntry(ARCHIVE, 1, 70, &size, 0);
        buffer[0] = 0;
        buffer[1] = 0;
        UploadObjPaletteBoth(buffer, 0, &size);
        Overlay5Display_LoadBgPalette(DISPLAY_ENGINE_SUB, buffer, 0, size);
        DeleteBuffer(buffer);
    }
    SixValues languages = data_ov007_0208d9f4;
    WORK.owned90 = func_ov005_02066f78(ARCHIVE, 1, (u16)languages.entries[gSaveData[0x515]], 0, 0);
    {
        void *buffer = Overlay5Archive_ReadEntry(ARCHIVE, 1, 64, &size, 0);
        Overlay5Display_LoadBgPalette(DISPLAY_ENGINE_SUB, buffer, 64, size);
        DeleteBuffer(buffer);
    }
    {
        void *buffer = Overlay5Archive_ReadEntry(ARCHIVE, 1, 71, &size, 0);
        Overlay5Display_LoadObjPalette(DISPLAY_ENGINE_MAIN, buffer, 0, size);
        Overlay5Display_LoadObjPalette(DISPLAY_ENGINE_SUB, buffer, 0, size);
        Overlay5Display_LoadBgPalette(DISPLAY_ENGINE_SUB, buffer, 128, 32);
        DeleteBuffer(buffer);
    }
    func_ov005_02067468(ARCHIVE, 0, data_ov007_0208d954[2], 1);
    if (gSaveData[0x515] == 0) {
        WORK.owned28 = Overlay5Archive_ReadEntry(ARCHIVE, 0, 3, 0, 0);
        WORK.owned2c = Overlay5Archive_ReadEntry(ARCHIVE, 0, 4, 0, 0);
    } else {
        WORK.owned28 = Overlay5Archive_ReadEntry(ARCHIVE, 0, 8, 0, 0);
        WORK.owned2c = Overlay5Archive_ReadEntry(ARCHIVE, 0, 4, 0, 0);
    }
    func_ov005_02067424(ARCHIVE, 0);
    WORK.image30 = WORK.owned28;
    WORK.palette34 = WORK.owned2c;
    WORK.image60 = WORK.owned28;
    WORK.palette64 = WORK.owned2c;
    WORK.owned80 = func_ov005_0206687c((const char *)data_ov007_0208d954[3], 0, 0);
    func_ov005_02067468(ARCHIVE, 2, data_ov007_0208d954[0], 1);
    {
        int i;
        AssetTable *table = (AssetTable *)Overlay5Archive_ReadEntry(ARCHIVE, 2, 0, 0, 1);
        i = 0;
        do {
            Overlay5Asset *asset = func_ov005_02066a7c(ARCHIVE, 2, (u16)resource_entries.entries[i].index,
                                                       table, (u8)resource_entries.entries[i].mode, 0);
            func_ov005_020698c8(i + 72, asset);
            ++i;
        } while (i < 12);
        Overlay5Asset *asset = func_ov005_02066a7c(ARCHIVE, 2, (u16)resource_entries.entries[12].index, table,
                                                   (u8)resource_entries.entries[12].mode, 0);
        func_ov005_020698c8(20, asset);
        DeleteBuffer(table);
    }
    void *party = GameHeap_New(4428, 0, 0, 0);
    if (party)
        party = PauseParty_Init(party, &WORK.image30);
    data_ov007_0208e1e4 = party;
}
}

extern "C" {
void func_ov005_02066358(PauseMenuElement *, void (*)(PauseMenuElement *), int);
void func_ov007_0206e27c(PauseMenuElement *);
}

extern "C" s32 PauseTransition_GetProgress(PauseSceneTask *unused_scene)
{
    return *(s32 *)data_ov007_0208e1e8;
}

extern "C" void PauseScene_RequestExit(PauseSceneTask *scene, int mode)
{
    scene->phase = 5;
    if (mode == 0)
        func_ov005_02066358(scene->menu,
            (void (*)(PauseMenuElement *))PauseScene_FadeOutTask, 0);
    else
        func_ov005_02066358(scene->menu, func_ov007_0206e27c, 0);
}

#include <game/pause_background.h>
extern "C" void func_ov005_0206650c(PauseBackgroundTask *);
static inline PauseBackgroundTask *CreateBackgroundTask(void (*callback)(PauseBackgroundTask *), int group)
{
    return (PauseBackgroundTask *)func_ov005_0206659c((void (*)(PauseMenuElement *))callback, group, 1);
}
extern "C" void DC_FlushRange(const void *, u32);

extern "C" void PauseBackground_LoadPalette(PauseBackgroundTask *task)
{
    DC_FlushRange(task->buffer, 512);
    Overlay5Display_BeginLoadBgExtPalette(DISPLAY_ENGINE_MAIN);
    Overlay5Display_LoadBgExtPalette(DISPLAY_ENGINE_MAIN, task->buffer, (task->selection << 9) + 16384, 512);
    Overlay5Display_LoadBgExtPalette(DISPLAY_ENGINE_MAIN, task->buffer, (task->selection << 9) + 24576, 512);
    Overlay5Display_EndLoadBgExtPalette(DISPLAY_ENGINE_MAIN);
    if (task->buffer) {
        GameHeap_DeleteArray(task->buffer);
        task->buffer = 0;
    }
    func_ov005_0206650c(task);
}
extern "C" PauseBackgroundTask *PauseBackground_QueuePalette(void *palette, int bank)
{
    PauseBackgroundTask *task = CreateBackgroundTask(PauseBackground_LoadPalette, 11);
    task->buffer = palette;
    task->selection = bank;
    return task;
}
