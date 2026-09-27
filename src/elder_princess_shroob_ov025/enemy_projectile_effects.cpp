/*
 * Elder Princess Shroob: enemy projectile effects (overlay 25, 0x020C5374-0x020C5DA4).
 *
 * Starting the effects that accompany a projectile, waiting for their animation
 * and releasing them again, then moving the spinning orb between projectiles.
 */

#include "effect_task_internal.h"
#include <game/battle_impact_effect.h>
extern "C" {

void Overlay25Enemy_BeginProjectileEffects(Overlay25Task *task, BattleSceneObject *object,
                                           Overlay25WorkPrefix *)
{
    BattleSceneObject *linked = BattleSceneObject_GetById(40);
    BattleSound_Play(97, 20, 0, 0);
    BattleSceneObject_SetAnimation(object, 25, -1);
    BattleSceneObject_AdjustPosition(linked, object->x - 20 - linked->x, object->y + 4 - linked->y,
                                     object->z + 40 - object->property_103 - linked->z);
    BattlePosition pos, origin;
    Overlay25Object_GetViewPosition(&pos, linked);
    Overlay25Object_GetViewPosition(&origin, object);
    BattleSpriteEffect_Spawn(525, pos.x, pos.y, pos.z, 256);
    BattleModelEffect_Spawn(821, object, pos.x - origin.x, pos.y - origin.y, pos.z - origin.z, 256);
    task->update = Overlay25Enemy_WaitProjectileEffectAnimation;
}

void Overlay25Enemy_WaitProjectileEffectAnimation(Overlay25Task *task, BattleSceneObject *object,
                                                  Overlay25WorkPrefix *work)
{
    BattleSceneObject *linked = BattleSceneObject_GetById(40);
    if (object->primary_model->flag_bits.panel_animation_trigger) {
        BattleSceneObject_SetAnimation(object, 26, -1);
        BattleSceneObject_SetAnimation(linked, 0, -1);
        linked->property_103 = -8;
        BattleSceneObject_AdjustPosition(linked, object->x - 20 - linked->x, object->y + 4 - linked->y,
                                         object->z + 40 - object->property_103 - linked->z);
        BattlePosition pos;
        Overlay25Object_GetViewPosition(&pos, linked);
        BattleSpriteEffect_Spawn(526, pos.x, pos.y, pos.z, 256);
        BattleModelEffect_SpawnAttached(&work->model_effect, 822, linked, 0, 0, 0, 256);
        task->update = Overlay25Enemy_ReleaseProjectileEffects;
        BattleSound_Play(96, 0, 0, 0);
    }
}

void Overlay25Enemy_ReleaseProjectileEffects(Overlay25Task *task, BattleSceneObject *object,
                                             Overlay25WorkPrefix *work)
{
    Overlay25Parameters *parameters = &task->parameters;
    BattleSceneObject *linked = BattleSceneObject_GetById(40);
    if (!work->model_effect) {
        BattleSound_Play(123, 0, 0, 0);
        BattleSceneObject_SetAnimation(object, 27, -1);
        parameters->index = -1;
        BattlePosition pos, origin;
        Overlay25Object_GetViewPosition(&pos, linked);
        Overlay25Object_GetViewPosition(&origin, object);
        BattleSpriteEffect_Spawn(527, pos.x, pos.y, pos.z, 256);
        BattleModelEffect_Spawn(823, object, pos.x - origin.x, pos.y - origin.y, pos.z - origin.z, 256);
        BattleImpactEmitter_Start(40, 0, 4096, 1, 1, 16, 16, 64, 64, 0);
        BattleImpactEmitter_Start(41, 0, 4096, 1, 1, 8, 8, 64, 64, 0);
        BattleEntity_BindResource(42, 52);
        BattleSceneObject_SetAnimation(BattleSceneObject_GetById(42), 0, 1);
        BattleSceneObject *projectile = BattleSceneObject_GetById(42);
        BattleSceneObject_AdjustPosition(projectile, -64 - projectile->x, -64 - projectile->y,
                                         -projectile->z);
        BattleModelAnimation_SetModels(0, (BattleModel *)-1,
            BattleSceneObject_GetActiveModel(BattleSceneObject_GetById(42)), (BattleModel *)-1);
        work->tasks[7].update = Overlay25Enemy_ResetAnimation;
        task->update = Overlay25Projectile_UpdateEmission;
    }
}

void Overlay25Enemy_ResetAnimation(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *)
{
    BattleSceneObject_GetById(40);
    if (object->primary_model->flag_bits.panel_animation_trigger) {
        BattleSceneObject_SetAnimation(object, 0, -1);
        task->update = 0;
    }
}

void Overlay25Projectile_UpdateEmission(Overlay25Task *task, BattleSceneObject *enemy,
                                        Overlay25WorkPrefix *work)
{
    Overlay25Parameters *parameters = &task->parameters;
    BattleSceneObject *orb = BattleSceneObject_GetById(40);
    orb->primary_model->rotation_z += 4096;
    BattlePosition pos;
    Overlay25Object_GetViewPosition(&pos, orb);
    s16 depth = pos.z;
    Overlay25Object_GetViewPosition(&pos, enemy);
    // Track the owner's projected depth with the native 256-unit offset.
    orb->effect_anchor_z += pos.z - depth - 256;
    if (task->parameters.index >= 0) {
        if (BattleSceneObject_IsAnimationActiveById(40, 2))
            return;
        BattleSound_Play(270, 0, 0, 0);
        BattleSceneObject *emitter = BattleSceneObject_GetById(41);
        BattlePosition emission;
        Overlay25Object_GetViewPosition(&emission, emitter);
        BattleModelAnimation_Start(824, 0, emission.x, emission.y, 0, 256);
        BattleModel *model = BattleSceneObject_GetActiveModel(
            BattleSceneObject_GetById((u16)(parameters->index + 50)));
        BattleModelAnimation_Start(825, model, 0, 0, 0, 256);
        int animation, effect;
        if (*(u32 *)(gBattleContext + 27132) & (1 << parameters->index)) {
            animation = 18;
            effect = 827;
        } else {
            animation = 16;
            effect = 826;
        }
        BattleSceneObject_SetAnimation(
            BattleSceneObject_GetById((u16)(parameters->index + 50)), animation, -1);
        BattleModelAnimation_Start(effect, model, 0, 0, 0, 256);
        Overlay25Parameters *child = &work->tasks[parameters->index + 1].parameters;
        child->timer = 0;
        child->parameter = effect;
        child->index = parameters->index;
        work->tasks[parameters->index + 1].update = Overlay25Projectile_UpdateTrail;
    }
    // Skip empty entries, then move to the next projectile or the final spin.
    do {
        if (++parameters->index == 6) {
            BattleSceneObject *last = BattleSceneObject_GetById((u16)(parameters->index + 44));
            // This final move uses slot 50's X directly, not an X difference.
            int dx = last->x;
            int dz = 192 - last->z;
            int distance_squared = dz * dz + dx * dx;
            *(vu16 *)0x040002b0 = 0;
            *(vu32 *)0x040002b8 = distance_squared;
            while (*(vu16 *)0x040002b0 & 0x8000) {
            }
            BattleSceneObject_MoveBy(
                orb, 2, dx, 0, dz, ((s32)*(vu32 *)0x040002b4 << 8) / 1024);
            task->update = Overlay25Projectile_WaitSpin;
            return;
        }
    } while (!OVERLAY25_PROJECTILE_COUNTS[parameters->index]);
    BattleSceneObject *projectile = BattleSceneObject_GetById((u16)(parameters->index + 44));
    int dx = projectile->x - orb->x;
    int dy = projectile->y - orb->y;
    int dz = projectile->z - orb->z;
    *(vu16 *)0x040002b0 = 0;
    *(vu32 *)0x040002b8 = dx * dx + dy * dy + dz * dz;
    while (*(vu16 *)0x040002b0 & 0x8000) {
    }
    BattleSceneObject_MoveBy(orb, 2, dx, dy, dz, ((s32)*(vu32 *)0x040002b4 << 8) / 1024);
}
}
