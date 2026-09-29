/*
 * Elder Princess Shroob: enemy chain landing (overlay 25, 0x020C85F0-0x020C8AC8).
 *
 * Returns the captured party member along the chain, applies landing damage,
 * and unlocks the party once the holding task finishes.
 */

#include "effect_task_internal.h"
#include <game/battle_object_link.h>
#include <hardware.h>

extern "C" {
#include <game/battle_damage.h>
#include <game/battle_status.h>
extern const s16 FX_SinCosTable_[];
void func_ov025_020c2eb0(Overlay25Task *, Overlay25WorkPrefix *, int);

void Overlay25Chain_ReturnParty(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *work)
{
    Overlay25Parameters *parameters = &task->parameters;
    Overlay25ChainState *chain = &work->chains[parameters->index];
    BattleActor *actor = BattleActor_GetPartySlot((u16)parameters->parameter);
    chain->unknown_302 = -1;
    // Retract at most one segment per frame, down to the configured floor.
    if (chain->unknown_31a < chain->countdown) {
        if (parameters->index == 2)
            parameters->timer -= 38;
        else
            parameters->timer -= 24;
        int count = (parameters->timer << 12) / (s16)parameters->unknown0e / chain->amplitude_q8;
        if (count < chain->countdown)
            --chain->countdown;
    }
    if (chain->unknown_318 < 2048)
        chain->unknown_318 += 32;
    u16 phase = 20480 - 20480 * FX_SinCosTable_[2*(parameters->mode_flags >> 4)+1] / 4096;
    int amplitude = -chain->amplitude_q8;
    int index = 2 * (phase >> 4);
    chain->joints[0].x = amplitude * FX_SinCosTable_[index] / 4096;
    chain->joints[0].y = 0;
    chain->joints[0].z = amplitude * FX_SinCosTable_[index+1] / 4096;
    BattleChain_RelaxSegments(chain->joints, chain->countdown, chain->amplitude_q8 / 256, chain->unknown_318);
    // Leave the final three segments beyond the party attachment point.
    s32 displacement[3];
    BattleObjectLink_SumSegments(displacement, (BattleObjectLinkSegment *)chain->joints, chain->countdown - 3);
    chain->endpoint_x_q8 = chain->x_q8 + displacement[0];
    chain->endpoint_y_q8 = chain->y_q8 + displacement[1];
    chain->endpoint_z_q8 = chain->z_q8 + displacement[2];
    BattleSceneObject_AdjustPosition(actor->scene_object,
        chain->endpoint_x_q8 / 256 - actor->scene_object->x,
        chain->endpoint_y_q8 / 256 - actor->scene_object->y,
        chain->endpoint_z_q8 / 256 - actor->scene_object->z);
    if (parameters->mode_flags < 0xfc00)
        parameters->mode_flags += 1024;
    if (parameters->mode_flags >= 0xfc00) {
        int id;
        if ((*(u32 *)(gBattleContext + 54176) << 15) >> 31)
            id = (u16)parameters->parameter == 56 ? 57 : 56;
        else
            id = (u16)parameters->parameter;
        // The battle variant can return the held member to the other home slot.
        BattleActor *target = BattleActor_GetPartySlot((u16)id);
        int dx = target->unk_018 - actor->scene_object->x;
        int dy = target->unk_01a - actor->scene_object->y;
        int dz = target->unk_01c - actor->scene_object->z;
        *rSQRTCNT = SQRTCNT_MODE_32;
        *rSQRT_PARAM_L = dx*dx + dy*dy + dz*dz;
        while (*rSQRTCNT & SQRTCNTF_BUSY) {}
        BattleSceneObject_MoveBy(actor->scene_object, 2, dx, dy, dz, (s32)(*rSQRT_RESULT << 8) / 2048);
        actor->scene_object->property_103 = 0;
        func_ov025_020c2eb0(&work->tasks[5] + (parameters->index & 1), work, parameters->index);
        task->update = Overlay25Chain_ApplyLandingDamage;
        BattleSound_Play(116, 0, 0, 0);
    }
}

void Overlay25Chain_ApplyLandingDamage(Overlay25Task *task, BattleSceneObject *enemy,
                                       Overlay25WorkPrefix *work)
{
    Overlay25Parameters *parameters = &task->parameters;
    BattleSceneObject *object = BattleSceneObject_GetById((u16)parameters->parameter);
    if (!BattleSceneObject_IsAnimationChannelActive(object, 2)) {
        int damage = BattleDamage_CalculateByObject(enemy->actor_id, (u16)parameters->parameter);
        BattleDamage_ApplyToParty(object, 0, 0, damage, 0, 0);
        BattleStatus_TryApply(BattleActor_GetById((u16)parameters->parameter), 2,
                              *(s16 *)(gBattleContext + 300), 5, 5);
        BattlePosition position;
        Overlay25Object_GetViewPosition(&position, object);
        BattleSpriteEffect_Spawn(815, position.x, position.y, position.z, 256);
        BattleModelEffect_SpawnAttached(&work->model_effect, 816, object, 0, 0, 0, 256);
        task->update = Overlay25Party_UnlockAfterTask;
    }
}

void Overlay25Party_UnlockAfterTask(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *work)
{
    Overlay25Parameters *parameters = &task->parameters;
    if (!work->tasks[5].update) {
        BattlePartyActor *actor = (BattlePartyActor *)BattleActor_GetPartySlot((u16)parameters->parameter);
        actor->state_flags.raw &= ~0x1000;
        task->update = 0;
    }
}
}
