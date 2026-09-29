#ifndef PIT_FIELD_SCENE_RELEASE_INTERNAL_H
#define PIT_FIELD_SCENE_RELEASE_INTERNAL_H

/*
 * The resources the field releases when it hands the screen to another scene.
 */
#include "field_scene_transition_internal.h"
#include <game/field_animation.h>
#include <game/field_timer.h>
extern "C" {
#include <game/heap.h>
u32 OS_DisableIrqMask(u32);
u32 OS_EnableIrqMask(u32);
}
static inline void ReleaseResources(FieldSystem *system) {
    FieldAreaContext *first, *second;
    u32 irq_mask;
    GameIrqTask_Disable(system->irq_task);
    if (system->irq_task)
        ((GameTaskDispatch *)system->irq_task)->delete_task();
    irq_mask = OS_DisableIrqMask(1);
    first = system->areas[0];
    second = system->areas[1];
    system->areas[0] = 0;
    system->areas[1] = 0;
    OS_EnableIrqMask(irq_mask);
    FieldArea_ReleaseRoomResources(first);
    if (first) {
        FieldArea_Destroy(first);
        GameHeap_Delete(first);
    }
    FieldArea_ReleaseRoomResources(second);
    if (second) {
        FieldArea_Destroy(second);
        GameHeap_Delete(second);
    }
    FieldSystem_ReleaseSharedResources(system);
    if (system->windows)
        ((GameTaskDispatch *)system->windows)->delete_task();
    if (system->sprite_animation) {
        FieldSpriteAnimation *sprites = system->sprite_animation;
        FieldSpriteAnimation_Destroy(sprites);
        GameHeap_Delete(sprites);
    }
    if (system->update_task)
        ((GameTaskDispatch *)system->update_task)->delete_task();
    if (system->archive)
        ((GameTaskDispatch *)system->archive)->delete_task();
    if (system->party_manager) {
        FieldPartyManager *parties = system->party_manager;
        FieldPartyManager_DestroyStorage(parties);
        GameHeap_Delete(parties);
    }
    if (system->model_animation) {
        FieldModelAnimation *models = system->model_animation;
        FieldModelAnimation_Destroy(models);
        GameHeap_Delete(models);
    }
    if (system->timer) {
        FieldTimer *timer = system->timer;
        FieldTimer_Destroy(timer);
        GameHeap_Delete(timer);
    }
}

#endif
