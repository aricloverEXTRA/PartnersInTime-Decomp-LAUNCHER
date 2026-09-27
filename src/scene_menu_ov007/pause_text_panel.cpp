/*
 * Pause text-panel motion and strips (overlay 7, 0x0207B658-0x0207B864).
 *
 * Panels accelerate toward a pixel target, then move offscreen when dismissed.
 * Child strips follow the Q12 position and retire after the parent closes.
 */
#include "pause_scene_internal.h"
#include <game/pause_text_panel.h>
#include <game/battle_scene.h>
#include <game/overlay005_resource.h>
#include <game/overlay007_party.h>

extern "C" {
u8 func_ov007_020750bc(Overlay7Party *);
BattleModel *Overlay5ResourceA_Get(PauseTextPanelTask *);
Overlay5ObjectSprite *Overlay5ResourceB_Get(PauseTextPanelTask *);
void func_ov005_0206650c(void *);
void func_ov005_02069084(void *, int);
}
#define PARTY ((Overlay7Party *)data_ov007_0208e1e4)

extern "C" void PauseTextPanel_Update(PauseTextPanelTask *task)
{
    BattleModel *model = Overlay5ResourceA_Get(task);
    if (task->phase < 1000) {
        if (PARTY->visible && !func_ov007_020750bc(PARTY))
            WORK.text_panel_active = 0;
        if (!WORK.text_panel_active)
            task->phase = 1000;
    }
    switch (task->phase) {
    case 0:
        task->velocity_x += task->acceleration_x;
        task->velocity_y += task->acceleration_y;
        task->x += task->velocity_x;
        task->y += task->velocity_y;
        --task->timer;
        if (!task->timer) {
            task->x = task->target_x * 4096;
            task->y = task->target_y * 4096;
            ++task->phase;
        }
        break;
    // Start moving away from the top or bottom edge.
    case 1000:
        task->velocity_y = 8 * 4096;
        if (task->upper)
            task->velocity_y = -task->velocity_y;
        ++task->phase;
        break;
    case 1001:
        task->y += task->velocity_y;
        if (task->y < -48 * 4096 || task->y > 240 * 4096) {
            task->closing = 1;
            func_ov005_0206650c(task);
            return;
        }
        break;
    }
    // Compute both pixel coordinates before updating the model.
    s16 x = task->x / 4096;
    s16 y = task->y / 4096;
    model->animation_offset_x = x;
    model->animation_offset_y = y;
    func_ov005_02069084(model, 11);
}

extern "C" void PauseTextPanel_UpdateStrip(PauseTextPanelTask *task)
{
    Overlay5ObjectSprite *sprite = Overlay5ResourceB_Get(task);
    PauseTextPanelTask *parent = task->parent;
    if (parent->closing) {
        func_ov005_0206650c(task);
        return;
    }
    sprite->x = parent->x;
    sprite->y = parent->y;
    func_ov005_02069084(sprite, 10);
}
