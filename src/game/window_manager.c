/*
 * Window manager (ARM9 resident, 0x0201F150-0x02020420).
 *
 * Opening, closing and pooling windows. Open takes a window out of the pool,
 * allocates its tiles and links it into its screen's list; Close runs the
 * closing animation and Release gives the tiles back. The construct/destroy pair
 * comes in Base and Complete forms depending on whether the manager owns the
 * buffers it draws into.
 */

#include <game/window.h>
void GameWindow_ResetProperties(GameWindowManager *manager, GameWindowProperties *properties) {
    properties->shape.bits.screen = 0;
    properties->shape.bits.skin = 1;
    properties->shape.bits.width = 0;
    properties->shape.bits.height = 0;
    properties->position.bits.tile_pitch = 0;
    properties->position.bits.tile_height = 0;
    properties->shape.bits.position_mode = 0;
    properties->shape.bits.sound = 0;
    properties->layout.bits.mode = 0;
    properties->layout.bits.flag4 = 0;
    properties->layout.bits.style = 0;
    properties->layout.bits.flag9 = 0;
    properties->layout.bits.width = 0;
    properties->layout.bits.extent = 0;
    properties->layout.bits.reserved28 = 0;
    properties->position.bits.x = 0;
    properties->position.bits.y = 0;
    properties->position.bits.reserved18 = 0;
    properties->value = 0;
    properties->fonts = 0;
    properties->string = 0;
    properties->reserved18 = 0;
}

#include <game/window_operations.h>

extern s16 func_0201d320(GameWindowManager *, const GameWindowProperties *, int);
extern void func_0201a714(GameWindowManager *, GameWindow *);

int GameWindow_Open(GameWindowManager *manager, const GameWindowProperties *properties,
                    int requested_index) {
    int index = func_0201d320(manager, properties, requested_index);
    if (index == -1)
        return -1;
    manager->windows[(s16)index].properties.shape.bits.position_mode = 0;
    GameWindow_WriteTilemap(manager, &manager->windows[(s16)index]);
    if ((u8)manager->windows[(s16)index].properties.shape.bits.screen)
        manager->state.bits.sub_map_dirty = 1;
    else
        manager->state.bits.main_map_dirty = 1;
    func_0201a714(manager, &manager->windows[(s16)index]);
    if (requested_index != -1)
        index = requested_index;
    return index;
}

void GameWindow_Close(GameWindowManager *manager, s16 index) {
    if (GameWindow_Release(manager, index)) GameWindow_RebuildTilemap(manager, (u8)manager->windows[index].properties.shape.bits.screen);
}

extern void func_0202cbd4(void *, int, u32);
extern char data_020565a4[];
static inline void InitLink(GameWindowLink *link) {
    MI_CpuFill8(link, 0, sizeof(*link));
    link->id = 0xffff;
}
void GameWindow_AllocatePool(GameWindowManager *manager, int heap, u32 count, int mode) {
    u32 window_bytes;
    u32 size;
    s16 i;
    if (!count) return;
    window_bytes = count * sizeof(GameWindow);
    size = window_bytes + (mode == 0 ? manager->main_capacity + manager->sub_capacity : count * sizeof(GameWindowTextCache));
    manager->windows = GameHeap_NewArray(size, heap, data_020565a4, 0);
    func_0202cbd4(manager->windows, 0, size);
    if (manager->main_capacity + manager->sub_capacity) manager->back_buffer = (u8 *)manager->windows + window_bytes;
    for (i = count - 1; i >= 0; --i) {
        InitLink(&manager->windows[i].links[0]);
        InitLink(&manager->windows[i].links[1]);
    }
    manager->state.bits.buffer_mode = mode;
    manager->window_capacity = count;
}

void GameWindow_ReleaseAll(GameWindowManager *manager) {
    s16 i;
    if (manager->windows) {
        for (i = manager->window_capacity - 1; i >= 0; --i) GameWindow_Release(manager, i);
        GameHeap_DeleteArray(manager->windows);
        manager->windows = 0;
        manager->window_capacity = 0;
    }
}
void GameWindow_SetOrigin(GameWindowManager *manager, int screen, s16 x, s16 y) {
    if (!screen) { manager->main_origin_x = x; manager->main_origin_y = y; }
    else { manager->sub_origin_x = x; manager->sub_origin_y = y; }
}
void GameWindow_Clear(GameWindowManager *manager, s16 index) {
    GameWindow_DrawSkin(manager, &manager->windows[index]);
    manager->windows[index].allocation.bits.suppress_redraw = 1;
}

#include <game/save_data.h>
void GameWindow_Pause(GameWindowManager *manager, s16 index) {
    GameWindow *window = &manager->windows[index];
    int id = index << 8;
    window->text_state.bits.paused = 1;
    GameSpriteEffectPool_Pause(manager->effects, (u16)id, 0);
    GameSpriteEffectPool_Pause(manager->effects, (u16)(id + 255), 0);
    if (window->has_selection) GameSpriteEffectPool_Pause(manager->effects, (u16)(id + (window->selection + 1)), 0);
}
void GameWindow_Resume(GameWindowManager *manager, s16 index) {
    GameWindow *window = &manager->windows[index];
    int id = index << 8;
    window->text_state.bits.paused = 0;
    GameSpriteEffectPool_Resume(manager->effects, (u16)id);
    GameSpriteEffectPool_Resume(manager->effects, (u16)(id + 255));
    if (window->has_selection) GameSpriteEffectPool_Resume(manager->effects, (u16)(id + (window->selection + 1)));
}

u32 GameWindow_MeasureWindowSize(GameWindowManager *manager, const u8 *string, const u32 *const *fonts)
{
    GameTextBounds bounds;
    u16 width, height;
    GameWindow_MeasureText(manager, string, fonts, &bounds, gSaveData[0x515]);
    width = bounds.width + 24;
    if (width & 1) ++width;
    width = (width >> 3) - 1;
    height = bounds.height + 16;
    if (height & 1) ++height;
    height = (height >> 3) - 1;
    return width + (height << 16);
}

extern void *data_02056534[];
extern u32 data_02056afc[];
extern u8 data_0205672c[], data_02056550[], data_02056560[], data_02056574[], data_02056584[],
    data_02056594[];

static inline GameWindowManager *Construct(GameWindowManager *manager, int priority, int unused, u8 main_bg,
                                           u8 sub_bg, u8 main_priority, u8 sub_priority,
                                           const GameWindowBuffers *buffers,
                                           const GameWindowTilemaps *tilemaps, int configure,
                                           GameSpriteAnimator *animator)
{
    u32 buffer_bytes, tilemap_bytes, size;
    GameSpriteEffectPool *effects;
    GameIrqTask *irq;
    GameTask_Init((GameTask *)manager, priority, unused, 0);
    manager->vtable = data_02056534;
    manager->tiles = data_02056afc;
    manager->palette_data = data_0205672c;
    manager->state.bits.main_bg = main_bg;
    manager->state.bits.sub_bg = sub_bg;
    manager->state.bits.main_priority = main_priority;
    manager->state.bits.sub_priority = sub_priority;
    if (configure == 1) {
        GameWindow_ConfigureMainBackground(manager, 9, 0);
        GameWindow_ConfigureSubBackground(manager, 9, 0);
    }
    buffer_bytes = 0;
    tilemap_bytes = 0;
    if (buffers) {
        manager->main_buffer = buffers->main;
        manager->sub_buffer = buffers->sub;
        manager->main_capacity = !buffers->main ? 0 : buffers->main_capacity;
        manager->sub_capacity = !buffers->sub ? 0 : buffers->sub_capacity;
    } else {
        manager->main_capacity = 18432;
        manager->sub_capacity = 18432;
        buffer_bytes = 36864;
    }
    if (tilemaps) {
        manager->main_tilemap = tilemaps->main;
        manager->sub_tilemap = tilemaps->sub;
        manager->main_map_size = !tilemaps->main ? 0 : tilemaps->main_size;
        manager->sub_map_size = !tilemaps->sub ? 0 : tilemaps->sub_size;
    } else {
        manager->main_map_size = 1920;
        manager->sub_map_size = 1920;
        tilemap_bytes = 3840;
    }
    manager->owned_buffer = 0;
    size = buffer_bytes + tilemap_bytes;
    if (size)
        manager->owned_buffer = GameHeap_NewArray(size, 0, data_02056550, 0);
    if (!buffers) {
        manager->main_buffer = manager->owned_buffer + tilemap_bytes;
        manager->sub_buffer = manager->main_buffer + 18432;
    }
    if (!tilemaps) {
        manager->main_tilemap = (u16 *)manager->owned_buffer;
        manager->sub_tilemap = (u16 *)(manager->owned_buffer + 1920);
    }
    if (!tilemaps) {
        func_0202cbd4(manager->main_tilemap, 0, 1920);
        func_0202cbd4(manager->sub_tilemap, 0, 1920);
    } else {
        if (tilemaps->main)
            func_0202cbd4(manager->main_tilemap, 0, 1920);
        if (tilemaps->sub)
            func_0202cbd4(manager->sub_tilemap, 0, 1920);
    }
    effects = GameHeap_New(40, 0, data_02056560, 0);
    if (effects)
        effects = GameSpriteEffectPool_Init(effects, 30);
    manager->effects = effects;
    manager->animator = animator;
    if (!animator) {
        animator = GameHeap_New(108, 0, data_02056574, 0);
        if (animator)
            animator = GameSpriteAnimator_Construct(animator);
        manager->animator = animator;
    }
    irq = GameHeap_New(40, 0, data_02056584, 0);
    if (irq)
        irq = GameWindowIrq_Init(irq, 8, (u32)data_02056594, manager);
    manager->irq_task = irq;
    manager->state.bits.enabled = 1;
    manager->display.bits.fixed_main_scroll = 0;
    manager->display.bits.fixed_sub_scroll = 0;
    manager->state.bits.main_map_dirty = 0;
    manager->state.bits.sub_map_dirty = 0;
    manager->state.bits.dirty = 0;
    manager->state.bits.main_count = 0;
    manager->state.bits.sub_count = 0;
    manager->main_origin_x = 0;
    manager->main_origin_y = 0;
    manager->sub_origin_x = 0;
    manager->sub_origin_y = 0;
    manager->main_scroll_x = 0;
    manager->main_scroll_y = 0;
    manager->sub_scroll_x = 0;
    manager->sub_scroll_y = 0;
    manager->windows = 0;
    manager->window_capacity = 0;
    manager->reservedf3c = 3843;
    manager->reservedf3e = 2;
    manager->display.bits.main_palette = 0;
    manager->display.bits.sub_palette = 0;
    manager->state.bits.main_bg_palette = 15;
    manager->state.bits.sub_bg_palette = 15;
    manager->main_head_marker = (GameWindow *)0x11111111;
    manager->main_windows = (GameWindow *)&manager->main_last;
    manager->main_last = (GameWindow *)&manager->main_head_marker;
    manager->main_tail_marker = (GameWindow *)-1;
    manager->sub_head_marker = (GameWindow *)0x11111111;
    manager->sub_windows = (GameWindow *)&manager->sub_last;
    manager->sub_last = (GameWindow *)&manager->sub_head_marker;
    manager->sub_tail_marker = (GameWindow *)-1;
    GameText_SetControl(11, 1, 0, 0);
    GameText_SetControl(12, 1, 0, 0);
    GameText_SetControl(15, 1, GameWindow_MeasureNumber, manager);
    GameText_SetControl(16, 1, GameWindow_MeasurePaddedNumber, manager);
    GameText_SetControl(17, 1, 0, 0);
    GameText_SetControl(18, 1, 0, 0);
    GameIrqTask_Enable(manager->irq_task);
    return manager;
}
GameWindowManager *GameWindow_ConstructBase(GameWindowManager *manager, int priority, int unused, u8 main_bg,
                                            u8 sub_bg, u8 main_priority, u8 sub_priority,
                                            const GameWindowBuffers *buffers,
                                            const GameWindowTilemaps *tilemaps, int configure,
                                            GameSpriteAnimator *animator)
{
    return Construct(manager, priority, unused, main_bg, sub_bg, main_priority, sub_priority, buffers,
                     tilemaps, configure, animator);
}
GameWindowManager *GameWindow_ConstructComplete(GameWindowManager *manager, int priority, int unused,
                                                u8 main_bg, u8 sub_bg, u8 main_priority, u8 sub_priority,
                                                const GameWindowBuffers *buffers,
                                                const GameWindowTilemaps *tilemaps, int configure,
                                                GameSpriteAnimator *animator)
{
    return Construct(manager, priority, unused, main_bg, sub_bg, main_priority, sub_priority, buffers,
                     tilemaps, configure, animator);
}

extern void GameWindow_ReleaseAll(GameWindowManager *);
/* The original delete path retains the inner null check after inlining. */
static inline void DeleteAnimator(GameSpriteAnimator *animator) {
    if (animator) { GameSpriteAnimator_Destroy(animator); GameHeap_Delete(animator); }
}
static inline void Destroy(GameWindowManager *manager) {
    GameSpriteAnimator *animator;
    GameSpriteEffectPool *effects;
    manager->vtable = data_02056534;
    GameIrqTask_DeleteSafe(manager->irq_task);
    animator = manager->animator;
    GameSpriteAllocation_Unlink(&animator->allocation[0]);
    animator->state.bits.main_allocated = 0;
    animator = manager->animator;
    GameSpriteAllocation_Unlink(&animator->allocation[1]);
    animator->state.bits.sub_allocated = 0;
    GameWindow_ReleaseAll(manager);
    animator = manager->animator;
    if (animator) DeleteAnimator(animator);
    effects = manager->effects;
    if (effects) { GameSpriteEffectPool_Destroy(effects); GameHeap_Delete(effects); }
    if (manager->owned_buffer) GameHeap_DeleteArray(manager->owned_buffer);
    GameTask_DestroyBase((GameTask *)manager);
}
GameWindowManager *GameWindow_DestroyComplete(GameWindowManager *manager) { Destroy(manager); return manager; }
GameWindowManager *GameWindow_Delete(GameWindowManager *manager) { Destroy(manager); GameHeap_Delete(manager); return manager; }
GameWindowManager *GameWindow_DestroyBase(GameWindowManager *manager) { Destroy(manager); return manager; }

extern void func_0201e79c(GameWindowManager *, GameWindow *);
extern void GameWindow_ScrollLink(GameWindowManager *, GameWindowLink *);
void GameWindow_Update(GameWindowManager *manager) {
    GameWindow *window;
    for (window = manager->main_windows; window->next != (GameWindow *)-1; window = window->next) {
        window->allocation.bits.suppress_redraw = 0;
        manager->display.bits.current_window = window->allocation.bits.group;
        if (window->text.text) func_0201e79c(manager, window);
        if (!window->properties.shape.bits.position_mode) {
            GameWindow_ScrollLink(manager, &window->links[0]);
            GameWindow_ScrollLink(manager, &window->links[1]);
        }
    }
    for (window = manager->sub_windows; window->next != (GameWindow *)-1; window = window->next) {
        window->allocation.bits.suppress_redraw = 0;
        manager->display.bits.current_window = window->allocation.bits.group;
        if (window->text.text) func_0201e79c(manager, window);
        if (!window->properties.shape.bits.position_mode) {
            GameWindow_ScrollLink(manager, &window->links[0]);
            GameWindow_ScrollLink(manager, &window->links[1]);
        }
    }
    GameSpriteEffectPool_Update(manager->effects);
    GameSpriteAnimator_Update(manager->animator);
    manager->main_scroll_x = manager->main_origin_x;
    manager->main_scroll_y = manager->main_origin_y;
    manager->sub_scroll_x = manager->sub_origin_x;
    manager->sub_scroll_y = manager->sub_origin_y;
}
