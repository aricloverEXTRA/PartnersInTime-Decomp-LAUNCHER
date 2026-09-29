/*
 * Elder Princess Shroob: chain target setup (overlay 25, 0x020C822C-0x020C85F0).
 *
 * Starts the attached effect, selects an eligible chain target, and waits
 * for the attached effect to finish.
 */

#include "effect_task_internal.h"

extern "C" {
#define CHAIN_TARGET_PAIR (*(u32 *)(gBattleContext + 27108))
#define CHAIN_TARGET_PHASE (*(u32 *)(gBattleContext + 27112))

void func_ov025_020c7b14(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);

void Overlay25Enemy_PrepareChainTarget(Overlay25Task *task, BattleSceneObject *enemy, Overlay25WorkPrefix *work)
{
    Overlay25Parameters *parameters = &task->parameters;
    switch (CHAIN_TARGET_PHASE) {
    case 0: {
        BattleSound_Play(0, 0, 0, 0);
        BattleGlobalProperty_Set(12, 0);
        BattleSceneObject_SetAnimation(enemy, 18, -1);
        BattlePosition position;
        Overlay25Object_GetViewPosition(&position, enemy);
        BattleSpriteEffect_Spawn(520, position.x, position.y, position.z, 256);
        BattleModelEffect_SpawnAttached(&work->model_effect, 816, enemy, 0, 0, 0, 256);
        task->update = Overlay25Enemy_FinishAttachedEffect;
        break;
    }
    case 1:
        BattleGlobalProperty_Set(12, 0);
        BattleActor_GetEnemySlot(enemy->actor_id)->damage_scale_q8 = 166;
        // Copy the raw target ID with the native unsigned halfword load.
        parameters->parameter = *(const u16 *)&BattleActor_GetEnemySlot(enemy->actor_id)->target_actor_id;
        if (CHAIN_TARGET_PAIR) {
            if ((u16)parameters->parameter == 56)
                parameters->index = ((*(u32 *)(gBattleContext + 54176) << 15) >> 31) != 0;
            else
                parameters->index = ((*(u32 *)(gBattleContext + 54176) << 15) >> 31) == 0;
        } else {
            if ((u16)parameters->parameter == 56)
                parameters->index = ((*(u32 *)(gBattleContext + 54176) << 15) >> 31) ? 2 : 3;
            else
                parameters->index = ((*(u32 *)(gBattleContext + 54176) << 15) >> 31) ? 3 : 2;
        }
        // Try the chosen chain, its partner, then the opposite pair.
        if (!BattleActor_CanReceiveStatus(BattleActor_GetById(
                (u16)work->chains[parameters->index].unknown_300)))
            parameters->index ^= 1;
        if (!BattleActor_CanReceiveStatus(BattleActor_GetById(
                (u16)work->chains[parameters->index].unknown_300)))
            parameters->index ^= 3;
        if (!BattleActor_CanReceiveStatus(BattleActor_GetById(
                (u16)work->chains[parameters->index].unknown_300))) {
            task->update = 0;
            return;
        }
        switch (parameters->index) {
        case 0:
            CHAIN_TARGET_PAIR = 1;
            parameters->parameter = 56;
            break;
        case 1:
            CHAIN_TARGET_PAIR = 1;
            parameters->parameter = 57;
            break;
        case 2:
            CHAIN_TARGET_PAIR = 0;
            parameters->parameter = 57;
            break;
        case 3:
            CHAIN_TARGET_PAIR = 0;
            parameters->parameter = 56;
            break;
        }
        parameters->angle = 0;
        parameters->mode = 0;
        BattleEntity_BindResource(40, enemy->resource->object_data_id);
        task->update = func_ov025_020c7b14;
        break;
    case 2:
        task->update = Overlay25Enemy_WaitAnimation;
        break;
    default:
        task->update = 0;
        break;
    }
    CHAIN_TARGET_PHASE = 1;
}

void Overlay25Enemy_FinishAttachedEffect(Overlay25Task *task, BattleSceneObject *object,
                                         Overlay25WorkPrefix *work)
{
    if (!work->model_effect) {
        BattleSceneObject_SetAnimation(object, 20, -1);
        task->update = 0;
    }
}
}
