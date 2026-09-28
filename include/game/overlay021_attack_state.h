#ifndef PIT_GAME_OVERLAY021_ATTACK_STATE_H
#define PIT_GAME_OVERLAY021_ATTACK_STATE_H

/*
 * The hammer attack: the primary and secondary swing phases and the input window
 * each one waits in.
 */
#include <game/battle_scene.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <game/battle_actor.h>
#ifdef __cplusplus
}
#endif
enum Overlay21AttackStateFlag {
    OVERLAY21_ATTACK_PRIMARY_PHASE_MASK = 0x1E0,
    OVERLAY21_ATTACK_SECONDARY_PHASE_MASK = 0x1E00,
    OVERLAY21_ATTACK_SECONDARY_PHASE_1 = 0x200,
    OVERLAY21_ATTACK_SECONDARY_PHASE_2 = 0x400
};

typedef struct Overlay21AttackConfig {
    u16 approach_duration, arrival_delay, primary_window, success_delay;
    u16 success_wait, success_return_duration, advance_duration, unknown0e;
    u16 unknown10, unknown12;
    s16 advance_animation_speed, advance_effect_interval;
    u16 early_wait, early_return_duration, miss_delay, miss_wait, miss_return_duration;
    s16 secondary_distance;
    u16 secondary_duration, phase_1_duration, phase_2_duration, secondary_window, status_chance;
    u16 unknown2e;
    int primary_power, secondary_power;
    s8 impact_x, impact_y, impact_z, secondary_impact_x;
    u32 resource;
} Overlay21AttackConfig;
typedef struct Overlay21AttackState {
    BattleSceneObject *object;
    BattlePartyActor *actor;
    Overlay21AttackConfig *config;
    int timer, primary_timer, secondary_timer;
    union {
        u32 flags;
        struct {
            u32 phase : 5, primary_phase : 4, secondary_phase : 4, secondary : 1, result : 4, unknown : 14;
        } bits;
    };
} Overlay21AttackState;
typedef struct Overlay21AttackWork {
    u8 unknown_00[17];
    s8 phase;
    u8 unknown_12[22];
    Overlay21AttackState attack;
} Overlay21AttackWork;
typedef char Overlay21AttackWork_SizeCheck[sizeof(Overlay21AttackWork)==68?1:-1];

typedef char Overlay21AttackConfig_SizeCheck[sizeof(Overlay21AttackConfig) == 64 ? 1 : -1];
typedef char Overlay21AttackState_SizeCheck[sizeof(Overlay21AttackState) == 28 ? 1 : -1];
#ifdef __cplusplus
extern "C" {
#endif
void Overlay21Attack_Advance(Overlay21AttackState *state);
void Overlay21Attack_UpdateInput(Overlay21AttackState *state);
/* The shared update forwards an advance flag; the hammer's impact ignores it. */
void Overlay21Attack_ApplyHit(Overlay21AttackState *state, int success, int advanced);
u32 Overlay21Attack_BeginReturn(Overlay21AttackState *state);
u32 Overlay21Attack_BeginApproach(Overlay21AttackState *state);
void Overlay21Attack_Create(BattlePartyActor *actor);
void Overlay21Attack_Update(BattlePartyActor *actor);
void Overlay21Attack_ConfigureAnimation(Overlay21AttackState *, int, int);
void Overlay21Attack_ClearPrimaryPhase(Overlay21AttackState *);
void Overlay21Attack_SetPrimaryPhase(Overlay21AttackState *, int, int);
void Overlay21Attack_StartSecondaryPhase(Overlay21AttackState *);
void Overlay21Attack_ClearSecondaryPhase(Overlay21AttackState *);
u32 Overlay21Attack_MissSecondary(Overlay21AttackState *state);
u32 Overlay21Attack_HitSecondary(Overlay21AttackState *state);
u32 Overlay21Attack_WaitSecondaryInput(Overlay21AttackState *state);
u32 Overlay21Attack_BeginSecondary(Overlay21AttackState *state);
Overlay21AttackState *Overlay21Attack_BeginReturnWait(Overlay21AttackState *state);
u32 Overlay21Attack_FinishAdvance(Overlay21AttackState *state);
u32 Overlay21Attack_BeginAdvance(Overlay21AttackState *state);
u32 Overlay21Attack_EarlyInput(Overlay21AttackState *state);
u32 Overlay21Attack_MissPrimary(Overlay21AttackState *state);
u32 Overlay21Attack_HitPrimary(Overlay21AttackState *state);
u32 Overlay21Attack_WaitPrimaryInput(Overlay21AttackState *state);
u32 Overlay21Attack_FinishApproach(Overlay21AttackState *state);
u32 Overlay21Attack_BeginEntry(Overlay21AttackState *state);
u32 Overlay21Attack_FinishReturn(Overlay21AttackState *state);
BattleSceneObject *Overlay21Attack_Initialize(Overlay21AttackState *state, BattlePartyActor *actor);
#ifdef __cplusplus
}
#endif
#endif
