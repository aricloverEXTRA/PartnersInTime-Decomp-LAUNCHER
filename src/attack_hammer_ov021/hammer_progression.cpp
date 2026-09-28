/*
 * Hammer progression (overlay 21, 0x020C3864-0x020C3CF0).
 * Entry and approach setup, followed by the per-frame animation, impact and
 * return transitions. Input processing runs before the current phase is read.
 */
extern "C" {
#include <game/battle_effect.h>
#include <game/battle_feedback.h>
#include <nitro/fx.h>
}
#include <game/overlay021_attack_state.h>

extern "C" {
extern Overlay21AttackConfig data_ov021_020c4030[];

static inline int CurrentPhase(const Overlay21AttackState *state)
{
    return state->bits.phase;
}

static inline u8 AnimationFinished(BattleModel *model)
{
    return model->flag_bits.unknown_09;
}

void Overlay21Attack_Advance(Overlay21AttackState *state)
{
    BattleSceneObject *object = state->object;
    BattleModel *model = BattleSceneObject_GetActiveModel(object);
    Overlay21AttackConfig *config = state->config;
    int phase;
    Overlay21Attack_UpdateInput(state);
    phase = CurrentPhase(state);
    switch (phase) {
    /* Wait for entry/approach animations and their optional arrival delay. */
    case 1:
        if (AnimationFinished(model)) Overlay21Attack_BeginApproach(state);
        break;
    case 2:
        if (!BattleSceneObject_IsAnimationChannelActive(object, 2))
            Overlay21Attack_FinishApproach(state);
        break;
    case 3:
        if (AnimationFinished(model)) {
            if (config->arrival_delay) {
                state->timer = config->arrival_delay;
                state->flags = (state->flags & ~31) | 4;
            } else Overlay21Attack_WaitPrimaryInput(state);
        }
        break;
    case 4:
        if (--state->timer <= 0) Overlay21Attack_WaitPrimaryInput(state);
        break;
    /* Apply each impact once, then wait for both the timer and animation. */
    case 6: case 7: case 10: case 15: case 16:
        if (state->timer > 0) --state->timer;
        else if (state->timer == 0) {
            int advanced, success;
            u32 result = state->flags;
            /* The native call interleaves the two predicates. MWCC schedules
             * the pure C comparisons serially and keeps phase in r2. */
            asm {
                cmp phase, #10
                mov result, result, lsl #14
                moveq advanced, #1
                mov result, result, lsr #28
                movne advanced, #0
                cmp result, #1
                moveq success, #1
                movne success, #0
            }
            Overlay21Attack_ApplyHit(state, success, advanced);
            state->timer = -1;
        }
        if (AnimationFinished(model) && state->timer == -1)
            Overlay21Attack_BeginReturnWait(state);
        break;
    /* Early input uses a separate advance and impact sequence. */
    case 8:
        if (AnimationFinished(model)) Overlay21Attack_BeginAdvance(state);
        break;
    case 9:
        if (!BattleSceneObject_IsAnimationChannelActive(object, 2))
            Overlay21Attack_FinishAdvance(state);
        else if (++state->timer >= config->advance_effect_interval) {
            BattleParty_PlayFormationSound(state->actor, 38, 39);
            state->timer = 0;
        }
        break;
    case 11:
        if (--state->timer <= 0) Overlay21Attack_BeginReturn(state);
        break;
    case 12:
        if (!BattleSceneObject_IsAnimationChannelActive(object, 2))
            Overlay21Attack_FinishReturn(state);
        break;
    case 13:
        if (AnimationFinished(model)) Overlay21Attack_WaitSecondaryInput(state);
        break;
    }
}

u32 Overlay21Attack_BeginEntry(Overlay21AttackState *state)
{
    BattleEntity_BindResource(state->object->actor_id, 52);
    Overlay21Attack_ConfigureAnimation(state, 0, 0);
    BattleSound_Play(50, 0, 0, 0);
    u32 result = (state->flags & ~31) | 1;
    state->flags = result;
    return result;
}

u32 Overlay21Attack_BeginApproach(Overlay21AttackState *state)
{
    BattleSceneObject *object = state->object;
    BattleSceneObject *target =
        BattleSceneObject_GetById((u16)state->actor->actor.target_actor_id);
    Overlay21AttackConfig *config =
        &data_ov021_020c4030[state->actor->formation_index - 2];
    Overlay21Attack_ConfigureAnimation(state, 1, 1);
    int dx = target->x + target->property_100 - object->x;
    int dy = target->y + target->property_101 - object->y;
    int distance = FX_Sqrt((dx * dx + dy * dy) << 12);
    BattleSceneObject_MoveBy(object, 2, dx, dy, 0,
                            distance * config->approach_duration / 0x80000);
    BattleSound_Play(61, 0, 0, 0);
    Overlay21Attack_SetPrimaryPhase(state, 1, 0);
    u32 result = (state->flags & ~31) | 2;
    state->flags = result;
    return result;
}
}
