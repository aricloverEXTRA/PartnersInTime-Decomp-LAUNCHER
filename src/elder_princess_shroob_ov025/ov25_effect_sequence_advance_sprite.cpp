/*
 * Elder Princess Shroob: effect sequence sprites (overlay 25, 0x020C3A08-0x020C4084).
 *
 * Starts an effect sequence's model or sprite and advances the sprite each
 * frame, then recalls active projectiles toward actor 43 and starts child tasks.
 */

#include "effect_task_internal.h"

extern "C" {
#include <game/random.h>
#include <nitro/fx_atan.h>

void func_ov025_020c37e4(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);

static inline void StartDistance3(int dx, int dy, int dz)
{
    *(vu16 *)0x040002b0 = 0;
    *(vu32 *)0x040002b8 = dx * dx + dy * dy + dz * dz;
    while (*(vu16 *)0x040002b0 & 0x8000) {
    }
}

void Overlay25EffectSequence_PositionEffect(Overlay25Task *task, BattleSceneObject *object,
                                            Overlay25WorkPrefix *work)
{
    BattleSceneObject *linked = BattleSceneObject_GetById(43);
    if (!work->model_effect) {
        BattleSceneObject_SetAnimation(object, 0, -1);
        BattleSceneObject_AdjustPosition(linked, 140 - linked->x, 128 - linked->y, -linked->z);
        BattleSceneObject_SetAnimation(linked, 0, -1);
        BattlePosition pos;
        Overlay25Object_GetViewPosition(&pos, linked);
        BattleSpriteEffect_Spawn(529, pos.x, pos.y, pos.z, 256);
        BattleModelEffect_SpawnAttached(&work->model_effect, 832, linked, 0, 0, 0, 256);
        linked->effect_anchor_z += 1024;
        task->update = Overlay25EffectSequence_StartModel;
        BattleSound_Play(309, 0, 0, 0);
    }
}

void Overlay25EffectSequence_StartModel(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *work)
{
    Overlay25Parameters *parameters = &task->parameters;
    BattleSceneObject *linked = BattleSceneObject_GetById(43);
    if (!work->model_effect) {
        parameters->effect = BattleModelEffect_Spawn(834, linked, 0, 0, 0, 256);
        parameters->effect->attached_script_flag = 1;
        if (parameters->mode == 1)
            task->update = Overlay25Enemy_PositionLoadedProjectiles;
        if (parameters->mode == 2)
            task->update = Overlay25EffectSequence_StartSprite;
    }
}

void Overlay25EffectSequence_StartSprite(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *work)
{
    BattleSceneObject *linked = BattleSceneObject_GetById(43);
    BattleModel *model = BattleSceneObject_GetActiveModel(BattleSceneObject_GetById(43));
    model->configure_animation_layer(0, 0, 1);
    model->animation_layer_states[0] = 0;
    task->parameters.timer = 0;
    linked->effect_anchor_z -= 1024;
    BattlePosition pos;
    Overlay25Object_GetViewPosition(&pos, linked);
    linked->effect_anchor_z += 1024;
    work->sprite_effect = BattleSpriteEffect_Spawn(530, pos.x, pos.y, pos.z, 256);
    task->update = Overlay25EffectSequence_AdvanceSprite;
    BattleSound_Play(394, 0, 0, 0);
}

void Overlay25EffectSequence_AdvanceSprite(Overlay25Task *task, BattleSceneObject *,
                                           Overlay25WorkPrefix *work)
{
    Overlay25Parameters *parameters = &task->parameters;
    BattleSceneObject *linked = BattleSceneObject_GetById(43);
    if (!work->sprite_effect->update_callback) {
        linked->effect_anchor_z -= 1024;
        BattlePosition pos;
        Overlay25Object_GetViewPosition(&pos, linked);
        linked->effect_anchor_z += 1024;
        work->sprite_effect = BattleSpriteEffect_Spawn(531, pos.x, pos.y, pos.z, 256);
        work->sprite_effect->sprite_flags |= 0x4000;
        if (parameters->mode == 1) {
            parameters->timer = 100;
            task->update = Overlay25EffectSequence_RecallProjectiles;
        }
        if (parameters->mode == 2)
            task->update = Overlay25EffectSequence_BeginSequentialLaunch;
    }
}

void Overlay25EffectSequence_RecallProjectiles(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *work)
{
    Overlay25Parameters *parameters = &task->parameters;
    BattleSceneObject *anchor = BattleSceneObject_GetById(43);
    int timer = parameters->timer;
    if (timer > 0)
        parameters->timer = timer - 1;
    // The update that reaches zero starts every active projectile together.
    if (parameters->timer <= 0) {
        for (int i = 0; i < 6; ++i) {
            if (OVERLAY25_PROJECTILE_COUNTS[i]) {
                BattleSceneObject *projectile = BattleSceneObject_GetById((u16)(i + 44));
                int dx = anchor->x - projectile->x;
                int dy = anchor->y - projectile->y;
                int dz = anchor->z - 24 - projectile->z;
                StartDistance3(dx, dy, dz);
                BattleSceneObject_StartScaledAcceleratedMotion(projectile, 2, dx, dy, dz,
                    *(vu32 *)0x040002b4, 0, 16, 1);
                Overlay25Task *next = &work->tasks[i + 1];
                Overlay25Parameters *child = &next->parameters;
                child->parameter = (*(u32 *)(gBattleContext + 27132) & (1 << i)) ? 57 : 56;
                child->index = i;
                child->timer = Random_NextModulo(8);
                child->angle = FX_Atan2Idx(dy - dz, dx);
                // Preserve the unsigned half-turn wrap before the 16-bit store.
                if (dx > 0)
                    child->angle = (u16)child->angle - 32768;
                BattleSceneObject_SetAnimation(projectile, (u16)child->parameter == 56 ? 17 : 19, -1);
                projectile->property_103 = -8;
                next->update = func_ov025_020c37e4;
            }
        }
        task->update = Overlay25EffectSequence_WaitParticleTasks;
        BattleSound_Play(254, 0, 0, 0);
    }
}

}
