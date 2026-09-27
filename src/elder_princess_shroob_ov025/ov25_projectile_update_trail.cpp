/*
 * Elder Princess Shroob: projectile trail (overlay 25, 0x020C4FD8-0x020C5374).
 *
 * Waits out the spin, launches active projectiles at staggered intervals,
 * and updates their animation trails.
 */

#include "effect_task_internal.h"

extern "C" {
#include <nitro/fx_atan.h>
extern int _s32_div_f(int, int);
void func_ov025_020c47c8(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);

static inline void StartDistance3(int x, int y, int z)
{
    *(vu16 *)0x040002b0 = 0;
    *(vu32 *)0x040002b8 = x * x + y * y + z * z;
    while (*(vu16 *)0x040002b0 & 0x8000) {
    }
}

void Overlay25Projectile_WaitSpin(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *)
{
    Overlay25Parameters *parameters = &task->parameters;
    BattleSceneObject *object = BattleSceneObject_GetById(40);
    object->primary_model->rotation_z += 4096;
    if (!BattleSceneObject_IsAnimationActiveById(40, 2)) {
        if (parameters->mode == 0) {
            parameters->index = -1;
            parameters->timer = 120;
            task->update = Overlay25Projectile_LaunchNext;
        }
        if (parameters->mode == 1) {
            parameters->index = -1;
            task->update = Overlay25EffectSequence_StartSprite;
        }
    }
}

void Overlay25Projectile_UpdateTrail(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *)
{
    Overlay25Parameters *parameters = &task->parameters;
    BattleSceneObject_GetById(40);
    if (++parameters->timer > 12) {
        parameters->timer = 0;
        BattleModel *model =
            BattleSceneObject_GetActiveModel(BattleSceneObject_GetById((u16)(parameters->index + 50)));
        BattleModelAnimation_Start((u16)parameters->parameter, model, 0, 0, 0, 256);
    }
}

void Overlay25Projectile_LaunchNext(Overlay25Task *task, BattleSceneObject *enemy,
                                    Overlay25WorkPrefix *work)
{
    Overlay25Parameters *parameters = &task->parameters;
    int timer = parameters->timer;
    if (timer > 0)
        parameters->timer = timer - 1;
    if (parameters->timer > 0)
        return;
    do {
        if (++parameters->index == 6) {
            task->update = Overlay25Projectile_WaitAll;
            return;
        }
        parameters->timer = 60;
    } while (!OVERLAY25_PROJECTILE_COUNTS[parameters->index]);
    int index = parameters->index;
    Overlay25Task *next = &work->tasks[index + 1];
    Overlay25Parameters *child = &next->parameters;
    child->parameter = (*(u32 *)(gBattleContext + 27132) & (1 << index)) ? 57 : 56;
    child->index = index;
    child->timer = 0;
    BattleSceneObject *projectile = BattleSceneObject_GetById((u16)(index + 44));
    BattleActor *target = BattleActor_GetPartySlot((u16)child->parameter);
    int dx = target->unk_018 - projectile->x;
    int dy = target->unk_01a - projectile->y;
    int dz = target->unk_01c - projectile->z;
    // Scale horizontal deltas using the native 24-unit target-height offset.
    int scale = _s32_div_f(-(projectile->z << 12), dz + 24);
    dx = scale * dx / 4096;
    dy = scale * dy / 4096;
    child->angle = FX_Atan2Idx(dy - dz, dx);
    StartDistance3(dx, dy, dz);
    BattleSceneObject_StartScaledAcceleratedMotion(
        projectile, 2, dx, dy, dz, *(vu32 *)0x040002b4, 256, 13, 1);
    BattleSceneObject_SetAnimation(projectile, (u16)child->parameter == 56 ? 17 : 19, -1);
    if ((u32)(*(u32 *)(gBattleContext + 54176) << 15) >> 31)
        child->parameter = BattleActor_CanReceiveStatus(BattleActor_GetById(56)) ? 56 : 57;
    projectile->property_103 = -8;
    next->update = func_ov025_020c47c8;
    BattleHitDescriptor *hit = BattleHitDescriptor_Configure(
        projectile->actor_id, 56, 0, enemy->actor_id, 63);
    BattleHitDescriptor_SetStatus(hit, 2, 5, 5);
    projectile->flags.raw &= ~0x10000;
    BattleSound_Play(254, 0, 0, 0);
}
}
