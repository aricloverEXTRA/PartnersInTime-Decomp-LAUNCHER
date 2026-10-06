/*
 * ov002 animation frame predicate (0x0207197C-0x020719A4): reports whether
 * the model's current frame is the last frame of its current animation.
 */

#include <nitro.h>
#include <game/battle_scene.h>

u16 func_02009224(BattleModel *model, s16 animation_id);

int func_ov002_0207197c(BattleModel *model)
{
    s16 frame = model->property_056;
    int len = func_02009224(model, -1);
    return len == frame + 1;
}
