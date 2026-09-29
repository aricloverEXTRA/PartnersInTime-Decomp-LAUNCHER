/*
 * Cannonballer setup (overlay 12, 0x020C5614-0x020C59E0).
 *
 * Allocates the attack's work area, resources and objects.
 */

#include "motion_internal.h"

extern "C" {
void Overlay12Attack_Initialize(BattlePartyActor *user)
{
    Overlay12AttackWork *work = (Overlay12AttackWork *)Overlay10Attack_CreateWork((Overlay10ActionActor *)user, 452, 1);
    BattleSceneObject *primary = user->actor.scene_object;
    BattleSceneObject *baby = BattleSceneObject_GetById(42);
    BattleSceneObject *partner = BattleSceneObject_GetById(user->linked_object_id);
    BattleSceneObject *partner_baby = BattleSceneObject_GetById(43);
    u8 formations[4];
    int formation = user->formation_index & 1;
    formations[0] = formation;
    formations[1] = formation + 2;
    formations[2] = formation ^ 1;
    formations[3] = (formation ^ 1) + 2;
    Overlay12Attack_InitializeController(&work->controllers[0], 0, formations[0], primary,
                                         BattleSceneObject_GetById(44),
                                         primary->resource->flags.bits.resource_index - 1);
    int baby_formation = formations[1];
    Overlay12Attack_InitializeController(&work->controllers[1], 1, baby_formation, baby,
                                         BattleSceneObject_GetById(45), 2480);
    int partner_formation = formations[2];
    Overlay12Attack_InitializeController(&work->controllers[2], 2, partner_formation, partner,
                                         BattleSceneObject_GetById(46),
                                         partner->resource->flags.bits.resource_index - 1);
    int partner_baby_formation = formations[3];
    Overlay12Attack_InitializeController(&work->controllers[3], 3, partner_baby_formation, partner_baby,
                                         BattleSceneObject_GetById(47), 2528);
    Overlay12Attack_InitializeObject(&work->state);
    work->parameter = data_ov012_020c5a48;
    BattleObjectData_AllocateLoadBuffer(52, 16384);
    BattleObjectData_AllocateLoadBuffer(53, 32768);
    BattleObjectData_AllocateLoadBuffer(54, 24576);
    BattleObjectData_AllocateLoadBuffer(55, 32768);
    BattleObjectData_AllocateLoadBuffer(56, 24576);
    BattleObjectData_AllocateLoadBuffer(57, 30720);
    BattleObjectData_AllocateLoadBuffer(58, 30720);
    BattleObjectData_ConfigureLoad(52, 2432, 1);
    BattleObjectData_ConfigureLoad(53, primary->resource->flags.bits.resource_index - 1, 1);
    BattleObjectData_ConfigureLoad(54, 2480, 1);
    BattleObjectData_ConfigureLoad(55, partner->resource->flags.bits.resource_index - 1, 1);
    BattleObjectData_ConfigureLoad(56, 2528, 1);
    BattleObjectData_ConfigureLoad(57, primary->resource->flags.bits.resource_index - 1, 1);
    BattleObjectData_ConfigureLoad(58, partner->resource->flags.bits.resource_index - 1, 1);
    BattleObjectData_EnsureLoaded(52, 0xc0000050);
    BattleObjectData_EnsureLoaded(53, data_ov012_020c5a54[formations[0]]);
    BattleObjectData_EnsureLoaded(54, data_ov012_020c5a54[baby_formation]);
    BattleObjectData_EnsureLoaded(55, data_ov012_020c5a54[partner_formation]);
    BattleObjectData_EnsureLoaded(56, data_ov012_020c5a54[partner_baby_formation]);
    BattleObjectData_EnsureLoaded(57, data_ov012_020c5a1c[formations[0] & 1]);
    BattleObjectData_EnsureLoaded(58, data_ov012_020c5a1c[partner_formation & 1]);
    BattleObjectData_AllocateLoadBuffer(59, 4096);
    BattleObjectData_ConfigureLoad(59, 2576, 0);
    BattleFeedback_LoadResource((int)func_ov002_02091fd8(59, 0xc1000051));
    *(u16 *)(gBattleContext + 54) = 0;
    *(u16 *)(gBattleContext + 52) = 63;
    *(u16 *)(gBattleContext + 50) = 6;
    *(u16 *)(gBattleContext + 48) = 12;
    if (Overlay10Party_HasBadgeTwo())
        Overlay12Attack_ApplyBadgeOffsets();
    Overlay10Attack_BeginEntry(user, func_ov012_020c5314);
    int i = 0;
    u32 formation_index;
    // Preserve the separate formation index used by the original loop.
    asm { mov formation_index, i }
    do
    {
        const s16 *home = data_ov012_020c5a3c[formations[formation_index] & 1];
        work->home[i][0] = home[0];
        work->home[i][1] = home[1];
        work->home_z[i] = home[2];
        ++i;
        formation_index += 2;
    }
    while (i < 2)
        ;
}
}
