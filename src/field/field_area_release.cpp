/* Release a complete room: entities, display resources and room-owned buffers. */

extern "C" {
#include <game/heap.h>
}
#include <game/field_area.h>
#include <game/field_background.h>
#include <game/field_timed_renderer.h>
#include <game/window.h>
#include <game/task.h>
#include "screen_wipe_internal.h"
extern "C" {
void func_02009058(int screen);
}

/* Release the room including its entities and room-owned data. */
extern "C" void FieldArea_ReleaseRoomResources(FieldAreaContext *area)
{
    FieldResourceContext *resources = (FieldResourceContext *)area;
    func_02009058((u8)area->flags.screen);
    /* The sub-screen tile allocation is embedded at area +8. */
    if (area->flags.screen == 1)
        GameSpriteAllocation_Unlink((GameSpriteAllocation *)area->unknown_0008);
    FieldArea_ClearNotifications(area);
    FieldArea_CloseMessageWindows(area, -1);
    if (area->window_slide.flags.active) {
        area->window_slide.flags.active = 0;
        FieldArea_SetMessageWindowClipEnabled(area, -1, 0);
    }
    if (!area->flags.screen) {
        GameSpriteAnimator *animator = ((GameWindowManager *)area->owner)->animator;
        GameSpriteAllocation_Unlink(&animator->allocation[0]);
        animator->state.raw &= ~0x10000;
    } else {
        GameSpriteAnimator *animator = ((GameWindowManager *)area->owner)->animator;
        GameSpriteAllocation_Unlink(&animator->allocation[1]);
        animator->state.raw &= ~0x20000;
    }
    void *crossfade = area->palette_crossfade;
    area->palette_crossfade = 0;
    GameHeap_Delete(crossfade);
    FieldScreenWipe *wipe = (FieldScreenWipe *)area->unknown_2bc8;
    /* Preserve the native clear-before-cleanup order. */
    area->unknown_2bc8 = 0;
    if (wipe->flags.active || wipe->flags.retain)
        FieldScreenWipe_Clear(area);
    GameHeap_Delete(wipe);
    GameHeap_DeleteArray(area->layer_motion);
    GameHeap_DeleteArray(area->quad_regions);
    GameHeap_DeleteArray(area->navigation_surfaces);
    GameHeap_DeleteArray(area->boundaries);
    for (int i = 0; i < area->entity_count; ++i) {
        if (area->entities[i]) area->entities[i]->base.unknown_04();
    }
    GameHeap_DeleteArray(resources->extra_room_buffer);
    for (int i = 0; i < 19; ++i)
        GameHeap_DeleteArray(resources->room_buffers[i + 1]);
    for (int set = 0; set < 2; ++set) {
        for (int i = 0; i < resources->animation_counts[set]; ++i)
            GameHeap_DeleteArray(resources->animation_buffers[set][i]);
        GameHeap_DeleteArray(resources->animation_buffers[set]);
    }
    for (int set = 0; set < 2; ++set) {
        for (int i = 0; i < resources->alternate_counts[set]; ++i) {
            GameSpritePalette_Unlink(&resources->alternate_palettes[set][i].palette);
            GameHeap_DeleteArray(resources->alternate[set][i].data);
            GameHeap_DeleteArray(resources->alternate[set][i].auxiliary);
        }
    }
    for (int set = 0; set < 2; ++set) {
        for (int i = 0; i < resources->secondary_counts[set]; ++i) {
            GameSpritePalette_Unlink(&resources->palettes[set][i].palette);
            GameHeap_DeleteArray(resources->secondary[set][i].data);
            GameHeap_DeleteArray(resources->secondary[set][i].auxiliary);
        }
    }
    for (int set = 0; set < 2; ++set) {
        for (int i = 0; i < resources->primary_counts[set]; ++i) {
            if (!resources->primary[set][i].bits.unknown_00) {
                GameHeap_DeleteArray(resources->primary[set][i].bounds);
                GameHeap_DeleteArray(resources->primary[set][i].graphics);
                GameHeap_DeleteArray(resources->primary[set][i].animation);
            }
        }
        GameHeap_DeleteArray(resources->primary[set]);
        GameHeap_DeleteArray(resources->secondary[set]);
        GameHeap_DeleteArray(resources->alternate[set]);
        GameHeap_DeleteArray(resources->palettes[set]);
        GameHeap_DeleteArray(resources->alternate_palettes[set]);
    }
    if (area->unknown_2534)
        ((GameTaskDispatch *)area->unknown_2534)->delete_task();
    area->unknown_2534 = 0;
    if (area->background)
        area->background->unknown_04();
    area->background = 0;
    for (int i = 0; i < 23; ++i) {
        if (area->effect_models[i]) {
            if (area->effect_models[i]->state_flag_bits.render_linked)
                area->effect_models[i]->stop();
            if (area->effect_models[i]->texture_offsets) {
                GameHeap_DeleteArray((void *)area->effect_models[i]->texture_offsets);
                area->effect_models[i]->texture_offsets = 0;
            }
            if (area->effect_models[i])
                area->effect_models[i]->delete_self();
        }
    }
    FieldRenderList_Clear(area->flags.screen);
    for (int i = 0; i < 2; ++i)
        GameSpritePalette_Unlink(&area->effect_palettes[i].palette);
    GameSpritePalette_Unlink(&area->effect_palettes[2].palette);
    GameHeap_DeleteArray(area->paired_bounds);
    GameHeap_DeleteArray((void *)area->variable_records);
    GameHeap_DeleteArray(resources->scripts[0]);
    GameHeap_DeleteArray(resources->scripts[1]);
}
