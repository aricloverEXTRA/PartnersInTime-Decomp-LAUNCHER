#ifndef PIT_GAME_WINDOW_H
#define PIT_GAME_WINDOW_H
#include <game/text.h>
#include <game/sprite_animator.h>
#include <game/sprite_effect.h>
#include <game/task.h>
/* Message and menu windows. A GameWindowManager owns one pool of windows per
   screen, the tile and tilemap memory they are drawn into, and the IRQ task
   that uploads the dirty parts during VBlank. Individual windows are addressed
   by a small index, not by pointer, so a scene can open, pause and close them
   without tracking allocations.

   Drawing is deferred throughout: writing to a window marks it dirty, the
   manager rebuilds the affected tiles, and the IRQ task pushes them to VRAM. */

/* The frame graphics: how thick the border is and the tile runs that make up
   its horizontal and vertical edges. */
typedef struct GameWindowSkin {
    u16 border_x, border_y;
    u8 reserved04[20];
    const s16 *horizontal;
    const s16 *vertical;
} GameWindowSkin;
/* What a window should look like, filled in by the caller before opening it.
   Packed into four words because the original code copies them as words. */
typedef struct GameWindowProperties {
    union { u32 raw; struct { u32 screen:1, skin:4, width:5, height:5, position_mode:3, sound:14; } bits; } shape;
    union { u32 raw; struct { u32 mode:4, flag4:1, style:4, flag9:1, width:8, extent:10, reserved28:4; } bits; } layout;
    union { u32 raw; struct { u32 x:9, y:9, reserved18:1, tile_pitch:6, tile_height:6, reserved31:1; } bits; } position;
    s32 value;
    const u32 *const *fonts;
    /* Direct string for extent 1023; otherwise an indexed text archive. */
    const u8 *string;
    u32 reserved18;
} GameWindowProperties;

/* Replay buffer for a window's text: the draw commands are recorded here so the
   window can be redrawn after a scroll or an effect without re-parsing the
   string. */
typedef struct GameWindowTextCache {
    u8 x, y, color, font;
    s16 scale_x, scale_y;
    u8 commands[200];
    u8 *cursor;
} GameWindowTextCache;

/* Ties a window to a scroll position, so it follows the background it sits on.
   Each window carries two, one per screen. */
typedef struct GameWindowLink {
    struct GameWindowLink *previous, *next;
    union { u32 raw; struct { u32 screen:1, reserved1:1, x:9, y:9, reserved20:6, linked:1, reserved27:4, fresh:1; } bits; } state;
    u16 id;
    u8 reserved0e[10];
} GameWindowLink;

/* A number being typed out one digit per frame. */
typedef struct GameWindowNumber {
    u8 text[8];
    const u8 *cursor;
    u8 ready;
    u8 reserved[3];
} GameWindowNumber;

/* One open window. `front` and `back` are the tile buffers it draws into;
   `links` are its two scroll attachments; the packed state words carry the
   dirty and closing flags the manager's update pass looks at. */
typedef struct GameWindow {
    struct GameWindow *previous, *next;
    GameWindowProperties properties;
    GameText text;
    GameWindowNumber number;
    union { u32 raw; struct { u8 origin_x, origin_y, reserved[2]; } bytes; struct { u32 reserved0:31, paused:1; } bits; } text_state;
    union { u32 raw; struct { u32 dirty:1, reserved1:2, suppress_redraw:1, reserved4:7, tile_offset:15, group:5, closing:1; } bits; } allocation;
    union { u32 raw; struct { u32 reserved0:1, size:16, animation:3, reserved20:2, layout_mode:5, reserved27:5; } bits; } state;
    u32 reserved70;
    u8 *front, *back;
    GameWindowLink links[2];
    u8 reservedac[20];
    u8 has_selection, selection;
    u16 measured_height;
    u8 sprite_slot;
    u8 reservedc5[7];
} GameWindow;
/* The owner: the window pool, both screens' tile and tilemap memory, the
   sprite animator and effect pool it draws through, and the per-screen lists of
   open windows with their scroll origins. Windows are kept on two intrusive
   lists bracketed by head and tail markers, which is why the list fields come
   in sets of four. */
typedef struct GameWindowManager {
    void **vtable;
    u8 reserved04[36];
    GameSpriteAnimator *animator;
    GameSpriteEffectPool *effects;
    u8 *owned_buffer;
    u16 *main_tilemap, *sub_tilemap;
    u8 *main_buffer, *sub_buffer;
    u32 main_capacity, sub_capacity;
    u16 main_map_size, sub_map_size;
    union { u32 raw; struct { u32 enabled:1, main_map_dirty:1, sub_map_dirty:1, dirty:1, main_count:5, sub_count:5, main_bg:2, sub_bg:2, main_priority:2, sub_priority:2, main_bg_palette:4, sub_bg_palette:4, buffer_mode:2; } bits; } state;
    union { u32 raw; struct { u32 main_palette:4, sub_palette:4, reserved8:17, fixed_main_scroll:1, fixed_sub_scroll:1, current_window:5; } bits; } display;
    const u32 *tiles;
    const void *palette_data;
    u8 reserved60[3744];
    GameWindow *windows;
    union { u8 *back_buffer; GameWindowTextCache *text_cache; };
    u16 window_capacity;
    u8 reservedf0a[2];
    GameWindow *main_head_marker;
    GameWindow *main_windows;
    GameWindow *main_last, *main_tail_marker, *sub_head_marker;
    GameWindow *sub_windows;
    GameWindow *sub_last, *sub_tail_marker;
    s16 main_scroll_x, main_scroll_y, sub_scroll_x, sub_scroll_y;
    s16 main_origin_x, main_origin_y, sub_origin_x, sub_origin_y;
    u16 reservedf3c, reservedf3e;
    GameIrqTask *irq_task;
} GameWindowManager;
typedef struct GameWindowBuffers {
    u8 *main, *sub;
    u32 main_capacity, sub_capacity;
} GameWindowBuffers;
typedef struct GameWindowTilemaps {
    u16 *main, *sub;
    u16 main_size, sub_size;
} GameWindowTilemaps;
typedef char GameWindowManagerSizeCheck[sizeof(GameWindowManager) == 3908 ? 1 : -1];
typedef char GameWindowBuffersSizeCheck[sizeof(GameWindowBuffers) == 16 ? 1 : -1];
typedef char GameWindowTilemapsSizeCheck[sizeof(GameWindowTilemaps) == 12 ? 1 : -1];
extern const GameWindowSkin *data_0205671c[];
typedef char GameWindow_SizeCheck[sizeof(GameWindow) == 204 ? 1 : -1];
typedef char GameWindowProperties_SizeCheck[sizeof(GameWindowProperties) == 28 ? 1 : -1];
typedef char GameWindowProperties_FontsOffsetCheck[
    (u32)&((GameWindowProperties *)0)->fonts == 16 ? 1 : -1];
typedef char GameWindowProperties_StringOffsetCheck[
    (u32)&((GameWindowProperties *)0)->string == 20 ? 1 : -1];
typedef char GameWindowLink_SizeCheck[sizeof(GameWindowLink) == 24 ? 1 : -1];
typedef char GameWindowTextCache_SizeCheck[sizeof(GameWindowTextCache) == 212 ? 1 : -1];
#ifdef __cplusplus
extern "C" {
#endif
/* ConstructComplete builds a manager that also allocates its own buffers;
   ConstructBase takes the buffers from the caller. */
GameWindowManager *GameWindow_ConstructComplete(GameWindowManager *manager, int priority, int unused,
    u8 main_bg, u8 sub_bg, u8 main_priority, u8 sub_priority,
    const GameWindowBuffers *buffers, const GameWindowTilemaps *tilemaps, int configure,
    GameSpriteAnimator *animator);
GameWindowManager *GameWindow_ConstructBase(GameWindowManager *manager, int priority, int unused,
    u8 main_bg, u8 sub_bg, u8 main_priority, u8 sub_priority,
    const GameWindowBuffers *buffers, const GameWindowTilemaps *tilemaps, int configure,
    GameSpriteAnimator *animator);
void GameWindow_ApplyScroll(GameWindowManager *manager);
void GameWindow_Upload(GameWindowManager *manager, GameWindow *window);
void GameWindow_UploadDirty(GameWindowManager *manager);
void GameWindow_UploadSubTilemap(GameWindowManager *manager);
void GameWindow_UploadMainTilemap(GameWindowManager *manager);
void GameWindow_ConfigureSubBackground(GameWindowManager *manager, int screen_base, int char_base);
void GameWindow_ConfigureMainBackground(GameWindowManager *manager, int screen_base, int char_base);
void GameWindow_RebuildTilemap(GameWindowManager *manager, int screen);
int GameWindow_Release(GameWindowManager *manager, s16 index);
void GameWindow_MeasureText(GameWindowManager *manager, const u8 *string, const u32 *const *fonts, GameTextBounds *bounds, u8 compact);
void GameWindow_AlignText(GameWindowManager *manager, GameWindow *window, GameText *text, int alignment);
void GameWindow_StartTextSound(GameWindowManager *manager, GameWindow *window);
GameIrqTask *GameWindowIrq_Delete(GameIrqTask *task);
GameIrqTask *GameWindowIrq_Destroy(GameIrqTask *task);
GameIrqTask *GameWindowIrq_Init(GameIrqTask *task, u32 priority, u32 unused, void *argument);
void GameWindowIrq_Update(GameIrqTask *task);
void GameWindow_ResetTextCache(GameWindowManager *manager, GameWindow *window);
void GameWindow_WriteTilemap(GameWindowManager *manager, GameWindow *window);
/* Allocate reserves a window and its tiles; Release gives both back. Close
   runs the closing animation first, Clear only blanks the contents. */
GameWindow *GameWindow_Allocate(GameWindowManager *manager, int screen, u32 size, s16 requested_index);
void GameWindow_ScrollLink(GameWindowManager *manager, GameWindowLink *link);
void GameWindow_RedrawAfterEffect(GameSpriteEffect *effect);
void GameWindow_Update(GameWindowManager *manager);
GameWindowManager *GameWindow_DestroyBase(GameWindowManager *manager);
GameWindowManager *GameWindow_Delete(GameWindowManager *manager);
GameWindowManager *GameWindow_DestroyComplete(GameWindowManager *manager);
void GameWindow_Resume(GameWindowManager *manager, s16 index);
void GameWindow_Pause(GameWindowManager *manager, s16 index);
void GameWindow_Clear(GameWindowManager *manager, s16 index);
void GameWindow_SetOrigin(GameWindowManager *manager, int screen, s16 x, s16 y);
void GameWindow_ReleaseAll(GameWindowManager *manager);
void GameWindow_AllocatePool(GameWindowManager *manager, int heap, u32 count, int mode);
void GameWindow_Close(GameWindowManager *manager, s16 index);
void GameWindow_ResetProperties(GameWindowManager *manager, GameWindowProperties *properties);
int GameWindow_MeasurePaddedNumber(void *argument, GameText *text, int process);
int GameWindow_MeasureNumber(void *argument, GameText *text, int process);
u32 GameWindow_MeasureWindowSize(GameWindowManager *manager, const u8 *string, const u32 *const *fonts);

#ifdef __cplusplus
}
#endif
#endif
