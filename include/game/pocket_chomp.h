#ifndef PIT_GAME_POCKET_CHOMP_H
#define PIT_GAME_POCKET_CHOMP_H
#ifdef __cplusplus
extern "C" {
#endif
#include <nitro/fx.h>
#ifdef __cplusplus
}
#endif
#include <game/battle_scene.h>
#include <game/overlay018_attack.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <game/battle_actor.h>
#ifdef __cplusplus
}
#endif
enum PocketChompPhase {
    POCKET_CHOMP_IDLE = 0,
    POCKET_CHOMP_EXIT = 4,
    POCKET_CHOMP_ENTRY = 6,
    POCKET_CHOMP_SMALL_BOUNCE = 7,
    POCKET_CHOMP_REVEAL = 8,
    POCKET_CHOMP_FALL = 9,
    POCKET_CHOMP_RUN = 11
};

enum PocketChompSupportPhase {
    POCKET_CHOMP_SUPPORT_IDLE = 0,
    POCKET_CHOMP_SUPPORT_READY = 1,
    POCKET_CHOMP_SUPPORT_ATTACK = 2,
    POCKET_CHOMP_SUPPORT_SWING = 4,
    POCKET_CHOMP_SUPPORT_ENTRY = 13
};

/* The two bounce modes retain independent Q12 velocities and accelerations.
   The rare-variant bit selects an animation bank; its roll is below 5 out of 100. */
typedef struct PocketChomp {
    BattleSceneObject *object;
    u8 party_snapshot[148];
    BattleSceneObject *linked_object;
    int timer, alternate_bounce_timer;
    int bounce_velocity_q12[2], bounce_acceleration_q12[2];
    int height_q12, velocity_q12;
    u16 squash_frame;
    union {
        u16 flags;
        struct {
            u16 phase : 5, direction : 1, launch_pending : 1, unknown_07 : 1, variant : 1,
                animation_locked : 1, rare_variant : 1, sound_played : 1, unknown12 : 4;
        } bits;
    };
} PocketChomp;
typedef struct PocketChompSupport {
    BattlePartyActor *actor;
    BattleSceneObject *object, *variant_object;
    int resource_index, timer;
    union {
        u32 flags;
        struct {
            u32 mode : 4, variant : 2, phase : 5, direction : 1, unknown12 : 20;
        } bits;
    };
    BattleSceneObject *linked_object;
    s8 particle_timer;
    struct {
        u8 unknown_00 : 1, unknown1 : 7;
    } action_flags;
    u16 unknown_1e;
} PocketChompSupport;

typedef struct PocketChompAdultMotion {
    BattlePartyActor *actor;
    BattleSceneObject *object, *variant_object;
    int resource_index, entry_timer;
    union {
        u32 flags;
        struct {u32 mode:4,variant:2,phase:5,direction:1,unknown12:20;} bits;
    };
    int bounce_count, input_timer;
    union {
        u32 timing_flags;
        struct {u32 grade:4,unknown04:28;} timing;
    };
} PocketChompAdultMotion;
typedef char PocketChompAdultMotion_SizeCheck[sizeof(PocketChompAdultMotion) == 36 ? 1 : -1];

typedef struct PocketChompTether {
    BattleSceneObject *target;
    s8 source_x, source_z, target_x, target_z;
} PocketChompTether;
typedef char PocketChompTether_SizeCheck[sizeof(PocketChompTether) == 8 ? 1 : -1];

typedef char PocketChomp_SizeCheck[sizeof(PocketChomp) == 192 ? 1 : -1];
typedef char PocketChompSupport_SizeCheck[sizeof(PocketChompSupport) == 32 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
int PocketChompAdult_GetClampedDistance(PocketChompAdultMotion *adult, PocketChomp *center);
void PocketChompAdult_Update(PocketChompAdultMotion *adult);
void PocketChompSupport_Update(PocketChompSupport *support, PocketChomp *center);
void PocketChomp_Update(PocketChomp *center, PocketChompAdultMotion *adult,
    PocketChompSupport *support, PocketChompSupport *other_support);
PocketChompTether *PocketChomp_StartTether(BattleSceneObject *object, int channel,
    BattleSceneObject *target, int source_x, int source_z, int target_x, int target_z);
void PocketChompAttack_ReturnBackdrop(void);
int PocketChompAttack_IsBackdropIdle(void);
int PocketChompAttack_AreAdultsIdle(void);
int PocketChompAttack_MoveAdultsOffscreen(void);
u32 PocketChompAdult_BeginExit(PocketChompAdultMotion *adult);
void PocketChompAdult_BounceAway(PocketChompAdultMotion *adult, int bounce);
u32 PocketChompAdult_BeginJump(PocketChompAdultMotion *adult);
void PocketChomp_UpdateVisibility(PocketChomp *center);
void PocketChomp_Boost(PocketChomp *center);
void PocketChomp_PreparePursuit(PocketChomp *center, s8 direction, int delay);
void PocketChomp_UpdateSquash(PocketChomp *center);
u32 PocketChomp_BeginFall(PocketChomp *center);
u32 PocketChomp_BeginSmallBounce(PocketChomp *center);
u32 PocketChomp_BeginEntry(PocketChomp *center, BattleSceneObject *target);
int PocketChomp_AttachObject(PocketChomp *center, int align);
int PocketChomp_GetAnchorOffset(s16 *offset, PocketChomp *center);
u32 PocketChomp_BeginExit(PocketChomp *center);
void PocketChomp_SetAnimation(PocketChomp *center, int animation, s8 facing);
int PocketChomp_PositionAtSide(PocketChomp *center, int target_id, int direction);
void PocketChomp_Hide(PocketChomp *center);
void PocketChomp_Initialize(PocketChomp *center, BattleSceneObject *linked);
int PocketChompSupport_UpdateParticles(PocketChompSupport *support);
void PocketChomp_UpdateLinkedAnimation(BattleSceneObject *object, PocketChomp *center);
void PocketChompSupport_SetLinkedAnimation(PocketChompSupport *support, int animation);
void PocketChompSupport_SpawnImpact(PocketChompSupport *support);
u32 PocketChompSupport_BeginSwing(PocketChompSupport *support);
void PocketChompSupport_StopLinkedMotion(PocketChompSupport *support);
void PocketChompSupport_Hide(PocketChompSupport *support);
void PocketChompSupport_Attach(PocketChompSupport *support, PocketChomp *center, int direction);
void PocketChompSupport_BeginAttack(PocketChompSupport *support);
void PocketChompSupport_ClearAnchor(PocketChompSupport *support);
void PocketChompSupport_Initialize(PocketChompSupport *support, s16 index, s8 mode, int resource);
int PocketChompAttack_ScaleTargetHeight(int height);
u32 PocketChompAdult_BeginEntry(Overlay18AttackObjectController *controller);
int PocketChompAdult_SelectEffect(Overlay18AttackObjectController *controller, int primary, int secondary);
void PocketChomp_Reveal(PocketChomp *center);
void PocketChomp_BeginRun(PocketChomp *center);
void PocketChomp_BeginExitWithSupport(PocketChomp *center, PocketChompSupport *support);
#ifdef __cplusplus
}
#endif
#endif
