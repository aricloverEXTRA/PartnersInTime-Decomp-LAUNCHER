#ifndef PARTNERS_IN_TIME_GAME_OVERLAY015_ATTACK_H
#define PARTNERS_IN_TIME_GAME_OVERLAY015_ATTACK_H

#include <game/battle_hit.h>
#include <game/battle_scene.h>

/* Prefix of the party actor used by this attack, not its allocation size. */
typedef struct Overlay15AttackModelOwner {
    BattleSceneObject *object;
    u8 unknown_04[0x7A];
    u16 formation_index;
} Overlay15AttackModelOwner;

typedef union Overlay15AttackPhaseFlags {
    u8 raw;
    struct {
        u8 phase : 4;
        u8 alternate_actor : 1;
        u8 unknown_5_7 : 3;
    } bits;
} Overlay15AttackPhaseFlags;

typedef union Overlay15AttackControllerFlags {
    u8 raw;
    struct {
        u8 phase : 5;
        u8 unknown_5_7 : 3;
    } bits;
} Overlay15AttackControllerFlags;

typedef struct Overlay15AttackHitMotion {
    BattleSceneObject *object;
    s32 animation_component;
    s32 terminal_velocity_q8;
    s32 duration_q8;
    s32 window_end_q8;
    s32 window_start_q8;
} Overlay15AttackHitMotion;

typedef struct Overlay15AttackModelController {
    Overlay15AttackModelOwner *primary;
    u8 unknown_04[4];
    u16 secondary_resource_id;
    u16 primary_resource_id;
    s16 hit_timer;
    s16 unknown_0e;
    s8 hit_active;
    Overlay15AttackControllerFlags phase_flags;
    u8 unknown_12[2];
    s16 animation_scale;
    s16 unknown_16;
    s16 unknown_18;
    u8 unknown_1a[2];
    Overlay15AttackHitMotion hit_motion;
} Overlay15AttackModelController;

typedef struct Overlay15AttackPhaseState {
    u8 unknown_00[0x16];
    Overlay15AttackPhaseFlags flags;
} Overlay15AttackPhaseState;

/* Prefix of the pair state; later native code also accesses offset 0x24. */
typedef struct Overlay15AttackObjectPairState {
    BattleSceneObject *primary;
    BattleSceneObject *secondary;
    u8 unknown_08[0xE];
    Overlay15AttackPhaseFlags flags;
    s8 launch_count;
    u16 target_actor_id;
    s16 pending_damage;
    s16 hit_count;
    u16 angle;
    s16 angle_step;
} Overlay15AttackObjectPairState;

/* Shared prefix of the item records accepted by the reward display. */
typedef struct Overlay15AttackRewardItemPrefix {
    u16 item_id;
    u16 name_id;
    u8 unknown_04[4];
    u8 kind;
    u8 animation_id;
} Overlay15AttackRewardItemPrefix;

typedef char Overlay15AttackModelOwner_SizeCheck[
    sizeof(Overlay15AttackModelOwner) == 0x80 ? 1 : -1];
typedef char Overlay15AttackModelController_SizeCheck[
    sizeof(Overlay15AttackModelController) == 0x34 ? 1 : -1];
typedef char Overlay15AttackHitMotion_SizeCheck[
    sizeof(Overlay15AttackHitMotion) == 0x18 ? 1 : -1];
typedef char Overlay15AttackObjectPairState_SizeCheck[
    sizeof(Overlay15AttackObjectPairState) == 0x24 ? 1 : -1];
typedef char Overlay15AttackRewardItemPrefix_SizeCheck[
    sizeof(Overlay15AttackRewardItemPrefix) == 10 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif

struct BattlePartyActor;
void Overlay15Attack_Initialize(struct BattlePartyActor *user);

void Overlay15Attack_ConfigureModelFlags(
    Overlay15AttackModelController *state);
int Overlay15Attack_ClampContextIndex(int offset);
void Overlay15Attack_EmptyHitCallback(BattleHitRecord *record);
void Overlay15Attack_ConfigureHitDescriptor(void);
int Overlay15Attack_IsLinkedPhase(Overlay15AttackPhaseState *state);
void Overlay15Attack_InitializeObjectPair(
    Overlay15AttackObjectPairState *state);
void Overlay15Attack_PreparePrimaryActor(
    Overlay15AttackModelController *state);
void Overlay15Attack_PrepareSecondaryActor(
    Overlay15AttackModelController *state);
void Overlay15Attack_UpdateHitWindow(Overlay15AttackModelController *state);
void Overlay15Attack_BeginSecondaryLaunch(Overlay15AttackModelController *state);
void Overlay15Attack_BeginSecondaryLaunchCallback(Overlay15AttackModelController *state);
void Overlay15Attack_BeginReverseAnimation(Overlay15AttackModelController *state);
int Overlay15Attack_IsIdle(Overlay15AttackModelController *state);
void Overlay15Attack_RestoreActors(Overlay15AttackModelController *state);
void Overlay15Attack_InitializeHitMotion(Overlay15AttackHitMotion *motion,
    BattleSceneObject *object, int animation_component);
void Overlay15Attack_InitializeActorController(Overlay15AttackModelController *state,
    Overlay15AttackModelOwner *owner, int resource_id, int alternate);
void Overlay15Attack_ApplyBadgeMotionAdjustments(void);
int Overlay15Attack_SelectRandomEnemy(void);
void Overlay15Attack_BeginPairRetreat(Overlay15AttackObjectPairState *state,
    Overlay15AttackModelController *actor);
void Overlay15Attack_HideAttackObjects(Overlay15AttackObjectPairState *state);
void Overlay15Attack_BeginPairLaunch(Overlay15AttackObjectPairState *state,
    int target_id, int damage, int alternate, int index_offset);
int Overlay15Attack_OpenRewardMessage(const Overlay15AttackRewardItemPrefix *item);
void Overlay15Attack_ShowRewardItem(const Overlay15AttackRewardItemPrefix *item,
    int message_width);
#ifdef __cplusplus
}
#endif

#endif
