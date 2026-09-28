extern "C" {

/*
 * Hammer attack lifecycle (overlay 21, 0x020C3CF0-0x020C3F80).
 *
 * Creates the attack, steps it each frame and finishes the return.
 */

#include <nitro/fx.h>
#include <game/battle_effect.h>
#include <game/heap.h>
#include <game/battle_context.h>
}
#include <game/overlay021_attack_state.h>
extern "C" {
extern Overlay21AttackConfig data_ov021_020c4030[];
extern void BattleParty_RestoreIdleAnimation(BattlePartyActor *, int);
extern Overlay21AttackWork *data_ov002_020c0710;
void func_0202cbd4(void *, int, u32);
int BattleFeedback_IsEffectComplete(void), BattleFeedback_IsResourceLoadPending(void);
void BattleFeedback_LoadResource(void);
void BattleAttack_SetCallback(BattlePartyActor *, void (*)(BattlePartyActor *));
enum {
    HAMMER_ATTACK_RESOURCE_SLOT = 52,
    HAMMER_RESOURCE_BUFFER_SIZE = 0xf000,
    HAMMER_CONTEXT_HEAP_OFFSET = 57684,
    HAMMER_CONTEXT_FLAGS_OFFSET = 54176,
    HAMMER_INPUT_FLAGS_OFFSET = 52096
};

void Overlay21Attack_Create(BattlePartyActor *actor)
{
    Overlay21AttackWork *work;
    Overlay21AttackConfig *config;
    int component;
    data_ov002_020c0710 = (Overlay21AttackWork *)GameHeap_Allocate(
        *(int *)(gBattleContext + HAMMER_CONTEXT_HEAP_OFFSET), sizeof(Overlay21AttackWork), 0, 1);
    func_0202cbd4(data_ov002_020c0710, 0, sizeof(Overlay21AttackWork));
    work = data_ov002_020c0710;
    *(u32 *)(gBattleContext + HAMMER_CONTEXT_FLAGS_OFFSET) &= ~0x1000;
    config = &data_ov021_020c4030[actor->formation_index - 2];
    Overlay21Attack_Initialize(&work->attack, actor);
    component = actor->actor.scene_object->resource->flags.bits.resource_index - 1;
    BattleObjectData_AllocateLoadBuffer(HAMMER_ATTACK_RESOURCE_SLOT, HAMMER_RESOURCE_BUFFER_SIZE);
    BattleObjectData_ConfigureLoad(HAMMER_ATTACK_RESOURCE_SLOT, component, 1);
    BattleObjectData_EnsureLoaded(HAMMER_ATTACK_RESOURCE_SLOT, config->resource);
    BattleFeedback_LoadResource();
    *(u16 *)(gBattleContext + HAMMER_INPUT_FLAGS_OFFSET) = 0;
    BattleAttack_SetCallback(actor, Overlay21Attack_Update);
}

void Overlay21Attack_Update(BattlePartyActor *actor)
{
    Overlay21AttackWork *work = data_ov002_020c0710;
    *(u16 *)(gBattleContext + HAMMER_INPUT_FLAGS_OFFSET) = 0;
    Overlay21Attack_Advance(&work->attack);
    switch (work->phase) {
    case 0:
        if (!BattleFeedback_IsResourceLoadPending()) {
            Overlay21Attack_BeginEntry(&work->attack);
            work->phase = 1;
        }
        break;
    case 1:
        if (!work->attack.bits.phase)
            work->phase = 2;
        break;
    case 2:
        if (BattleFeedback_IsEffectComplete()) {
            BattleAttack_SetCallback(actor, 0);
            data_ov002_020c0710 = 0;
            *(u16 *)(gBattleContext + HAMMER_INPUT_FLAGS_OFFSET) = 0;
        }
        break;
    }
}

BattleSceneObject *Overlay21Attack_Initialize(Overlay21AttackState *state, BattlePartyActor *actor)
{
    state->object = actor->actor.scene_object;
    state->actor = actor;
    state->config = &data_ov021_020c4030[actor->formation_index - 2];
    state->timer = 0;
    state->primary_timer = 0;
    state->secondary_timer = 0;
    state->bits.phase = 0;
    state->bits.primary_phase = 0;
    state->bits.secondary_phase = 0;
    state->bits.secondary = 0;
    BattleSceneObject *object = state->object;
    object->effect_anchor_z = -32;
    return object;
}

u32 Overlay21Attack_FinishReturn(Overlay21AttackState *state)
{
    state->object->effect_anchor_z = 0;
    BattleParty_RestoreIdleAnimation(state->actor, 0);
    u32 result = state->flags & ~31;
    state->flags = result;
    return result;
}
}
