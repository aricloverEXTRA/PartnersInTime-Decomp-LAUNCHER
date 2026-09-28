#ifndef PIT_GAME_OVERLAY025_EFFECT_TASK_H
#define PIT_GAME_OVERLAY025_EFFECT_TASK_H
#ifdef __cplusplus
extern "C" {
#endif
#include <nitro.h>
#include <game/battle_actor.h>
#include <game/battle_object.h>
#ifdef __cplusplus
}
#endif
#include <game/battle_effect.h>
typedef struct Overlay25ChainJoint {
    /* Q8 segment vector; relaxation retains full-width intermediates. */
    s16 x, y, z;
    union {
        s16 unknown_06[3];
        /* Cumulative original-minus-relaxed displacement, in whole units. */
        struct { s16 displacement_x, displacement_y, displacement_z; };
    };
} Overlay25ChainJoint;
typedef char Overlay25ChainJoint_DisplacementOffsetCheck[
    (u32)&((Overlay25ChainJoint *)0)->displacement_x == 6 ? 1 : -1];
typedef struct Overlay25ChainState {
    Overlay25ChainJoint joints[64];
    s16 unknown_300, unknown_302, unknown_304, countdown;
    s16 unknown_308, amplitude_q8, unknown_30c;
    s16 offset_x, offset_y, offset_z;
    s16 unknown_314, counter, unknown_318, unknown_31a;
    union {
        struct { u16 object_id; u8 unknown_31e[26]; };
        struct {
            u8 unknown_31c[4];
            /* Chain sum plus its world-space Q8 anchor. */
            s32 endpoint_x_q8, endpoint_y_q8, endpoint_z_q8;
            u8 unknown_32c[12];
        };
    };
    s32 x_q8, y_q8, z_q8;
} Overlay25ChainState;
typedef char Overlay25ChainState_EndpointOffsetCheck[
    (u32)&((Overlay25ChainState *)0)->endpoint_x_q8 == 800 ? 1 : -1];
typedef char Overlay25ChainState_SizeCheck[sizeof(Overlay25ChainState)==836?1:-1];
typedef struct Overlay25ChainMotionParameters {
    u16 phase;
    s16 phase_step, timer, angle;
    u8 flags[4], unknown_0c[20];
} Overlay25ChainMotionParameters;

typedef char Overlay25ChainJoint_SizeCheck[sizeof(Overlay25ChainJoint) == 12 ? 1 : -1];
typedef char Overlay25ChainMotionParameters_SizeCheck[sizeof(Overlay25ChainMotionParameters) == 32 ? 1 : -1];

typedef struct Overlay25ChainTrackingParameters {
    s16 index, target_id, timer, angle, mode, unknown_0a;
    u8 contact_count, unknown_0d[19];
} Overlay25ChainTrackingParameters;
typedef char Overlay25ChainTrackingParameters_SizeCheck[sizeof(Overlay25ChainTrackingParameters) == 32 ? 1 : -1];

typedef struct Overlay25Task Overlay25Task;
typedef struct Overlay25WorkPrefix Overlay25WorkPrefix;
typedef void (*Overlay25Callback)(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);
typedef struct Overlay25Parameters {
    s16 index, parameter, timer;
    union {
        s16 angle;
        struct {
            s8 travel_sound;
            u8 unknown0b;
        };
    };
    union {
        s16 mode;
        u16 mode_flags;
    };
    u16 unknown0e;
    BattleEffect *effect;
    s8 sound;
    u8 unknown15[15];
} Overlay25Parameters;
struct Overlay25Task {
    Overlay25Callback update;
    union {
        int sound_handle;
        Overlay25Parameters parameters;
        Overlay25ChainMotionParameters chain_motion;
        Overlay25ChainTrackingParameters chain_tracking;
    };
};
struct Overlay25WorkPrefix {
    /* Eight 836-byte chain workspaces; attack state chooses which half is used. */
    union { u8 unknown000[6688]; Overlay25ChainState chains[8]; };
    BattleEffect *model_effect, *sprite_effect;
    u32 unknown1a28;
    s16 phase, unknown1a2e;
    s16 slots[16];
    s16 secondary_slots[16];
    s16 slot_depth_offsets[16];
    Overlay25Task tasks[8];
};
typedef char Overlay25Parameters_SizeCheck[sizeof(Overlay25Parameters) == 32 ? 1 : -1];

typedef char Overlay25Task_SizeCheck[sizeof(Overlay25Task) == 36 ? 1 : -1];
typedef char Overlay25WorkPrefix_SizeCheck[sizeof(Overlay25WorkPrefix) == 7088 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
void Overlay25Enemy_UpdateLinkedEffectGrowth(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25Enemy_FeedLinkedEffect(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25Enemy_LaunchLinkedEffect(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25Enemy_WaitLinkedEffectAnimation(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *work);
void Overlay25Enemy_InitializeLinkedEffects(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *work);
void Overlay25Enemy_BeginLinkedEffects(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *work);
void Overlay25Projectile_CheckHit(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *work);
void Overlay25Enemy_EmitProjectile(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *work);
void Overlay25Enemy_BeginReturn(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *);
void Overlay25Enemy_UpdateReturn(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *work);
void Overlay25EffectSequence_BeginSequentialLaunch(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25EffectSequence_RecallProjectiles(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25EffectSequence_WaitParticleTasks(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25Task_WaitFirstChild(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25EffectSequence_InitializeAlternate(Overlay25Task *task, BattleSceneObject *object,
                                                 Overlay25WorkPrefix *);
void Overlay25EffectSequence_UpdateApproach(Overlay25Task *task, BattleSceneObject *object,
                                            Overlay25WorkPrefix *work);
void Overlay25EffectSequence_BeginPositioning(Overlay25Task *task, BattleSceneObject *object,
                                              Overlay25WorkPrefix *);
void Overlay25EffectSequence_FinishModelEffect(Overlay25Task *task, BattleSceneObject *,
                                               Overlay25WorkPrefix *work);
void Overlay25EffectSequence_FinishParticles(Overlay25Task *task, BattleSceneObject *,
                                             Overlay25WorkPrefix *work);
void Overlay25EffectSequence_AdvanceSprite(Overlay25Task *task, BattleSceneObject *,
                                           Overlay25WorkPrefix *work);
void Overlay25EffectSequence_StartSprite(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *work);
void Overlay25EffectSequence_StartModel(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *work);
void Overlay25EffectSequence_PositionEffect(Overlay25Task *task, BattleSceneObject *object,
                                            Overlay25WorkPrefix *work);
void Overlay25Projectile_UpdateReflectedImpact(Overlay25Task *task, BattleSceneObject *enemy, Overlay25WorkPrefix *work);
void Overlay25Projectile_UpdateSpin(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25Projectile_LaunchNext(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25Projectile_WaitAll(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *work);
void Overlay25Projectile_UpdateEmission(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25Projectile_UpdateTrail(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25Projectile_WaitSpin(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25Enemy_ReleaseProjectileEffects(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *work);
void Overlay25Enemy_WaitProjectileEffectAnimation(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *work);
void Overlay25Enemy_BeginProjectileEffects(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *work);
void Overlay25Enemy_ResetAnimation(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *);
void Overlay25EffectSequence_Initialize(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25Projectile_SpawnBurst798(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25Projectile_SpawnBurst799(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25Enemy_BeginLoadAndMove(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *);
void Overlay25Enemy_BeginPause(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *);
void Overlay25Enemy_UpdatePause(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *);
void Overlay25Enemy_BeginProjectilePhase(Overlay25Task *task, BattleSceneObject *object,
                                         Overlay25WorkPrefix *);
void Overlay25Enemy_WaitProjectiles(Overlay25Task *task, BattleSceneObject *object,
                                    Overlay25WorkPrefix *work);
void Overlay25Enemy_WaitReturnVariant(Overlay25Task *task, BattleSceneObject *object,
                                      Overlay25WorkPrefix *work);
void Overlay25Projectile_HideAndSignal(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25Enemy_BeginNextAnimation(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *);
void Overlay25Enemy_ResetStoppedAnimation(Overlay25Task *task, BattleSceneObject *object,
                                          Overlay25WorkPrefix *);
void Overlay25Enemy_BeginModelEffect(Overlay25Task *task, BattleSceneObject *object,
                                     Overlay25WorkPrefix *work);
void Overlay25Enemy_InitializeSegmentChains(Overlay25Task *task, BattleSceneObject *enemy, Overlay25WorkPrefix *work);
void Overlay25Chain_BeginTracking(Overlay25Task *task, BattleSceneObject *enemy, Overlay25WorkPrefix *work);
void Overlay25Enemy_SelectTrackingTargets(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25Enemy_WaitPair(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *work);
void Overlay25Enemy_PositionLoadedProjectiles(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *work);
void Overlay25Enemy_LoadProjectiles(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *work);
void Overlay25Enemy_WaitReturn(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *work);
void Overlay25Enemy_WaitAttachedEffect(Overlay25Task *task, BattleSceneObject *object,
                                       Overlay25WorkPrefix *work);
void Overlay25Task_WaitSibling(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *work);
void Overlay25Chain_RetractForRetry(Overlay25Task *task, BattleSceneObject *enemy, Overlay25WorkPrefix *work);
void Overlay25Chain_ReturnParty(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25Chain_ApplyLandingDamage(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25LinkedEffect_ApplyPartyLandingDamage(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25LinkedEffect_ApplyReflectedDamage(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25Party_UnlockAfterTask(Overlay25Task *task, BattleSceneObject *, Overlay25WorkPrefix *work);
void BattleChain_RelaxSegments(Overlay25ChainJoint *joint, int count,
                               int segment_length, int smoothing_q12);
void Overlay25Enemy_PrepareChainTarget(Overlay25Task *, BattleSceneObject *, Overlay25WorkPrefix *);
void Overlay25Enemy_FinishAttachedEffect(Overlay25Task *task, BattleSceneObject *object,
                                         Overlay25WorkPrefix *work);
void Overlay25Enemy_WaitAnimation(Overlay25Task *task, BattleSceneObject *object, Overlay25WorkPrefix *);
void Overlay25Enemy_EmitTravelEffects(Overlay25Task *task, BattleSceneObject *object,
                                      Overlay25WorkPrefix *work);
#ifdef __cplusplus
}
#endif
#endif
