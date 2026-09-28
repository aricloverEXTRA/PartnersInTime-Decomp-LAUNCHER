#ifndef PIT_OVERLAY012_MOTION_INTERNAL_H
#define PIT_OVERLAY012_MOTION_INTERNAL_H

/*
 * Declarations shared inside the Cannonballer overlay.
 */
extern "C" {
#include <nitro.h>
#include <game/battle_actor.h>
#include <game/battle_damage.h>
#include <game/overlay012_attack_state.h>
#include <game/overlay010_motion.h>
}
#include <game/battle_effect.h>

struct Overlay12AttackWork {
    int timer;
    u8 unknown_04[12];
    u8 flags;
    s8 phase;
    u8 unknown_12[22];
    BattlePartyActor *user;
    u8 unknown_2c[178];
    s16 current_hp[6];
    u8 unknown_ea[26];
    int parameter;
    u8 unknown_108[8];
    s16 home[2][2], home_z[2];
    u8 unknown_11c[20];
    Overlay12AttackController controllers[4];
    Overlay12AttackState state;
    u16 held_input, pressed_input;
    u32 damage_timer : 31, damage_pending : 1;
};
struct Overlay12ControllerOptions {
    u8 mode : 2, result : 2, launched : 1, unknown : 3;
};
typedef char Overlay12WorkSizeCheck[sizeof(Overlay12AttackWork) == 452 ? 1 : -1];
extern "C" {
extern Overlay12AttackWork *data_ov002_020c0710;
extern u8 *gBattleContext;
extern int data_ov012_020c5a04;
extern s16 data_ov012_020c5a3c[][3];
extern int data_ov012_020c5a48;
extern int data_ov012_020c5a54[], data_ov012_020c5a1c[];
extern u16 data_ov012_020c5a34[];
struct Overlay10ActionActor;
struct Overlay10Work;
Overlay10Work *Overlay10Attack_CreateWork(Overlay10ActionActor *, int, int);
int Overlay10Party_HasBadgeTwo();
void Overlay12Attack_ApplyBadgeOffsets();
void func_ov012_020c5314(BattlePartyActor *);
void Overlay10Attack_BeginEntry(BattlePartyActor *, void (*)(BattlePartyActor *));
void BattleFeedback_LoadResource(int);
int Overlay10Enemy_IsSelectable(int);
void Overlay10Enemy_PlayHitReaction(int, int);
void BattleFeedback_SpawnVariant(BattleSceneObject *, int, int, int);
void BattleMotion_StartTransform(BattleSceneObject *, int, int, int, int, int, int, int, int, int, int);
void func_ov012_020c3db8(Overlay12AttackController *);
void func_ov012_020c31e8(Overlay12AttackState *);
void BattleAttack_SetCallback(BattlePartyActor *, void (*)(BattlePartyActor *));
void func_ov012_020c50c0(BattlePartyActor *);
void Overlay12Motion_UpdateDampedArc(BattleSceneObject *, BattleMotionChannel *);

struct Overlay12ArcParameters {
    int acceleration, velocity;
    s16 start, end;
};
void func_ov012_020c2b48(int actor_id);
void Overlay12Attack_ShowHit(int actor_id, Overlay12AttackController *controller, int unused, int bonus);
int Overlay12Attack_HideObject(Overlay12AttackState *state);
void Overlay12Attack_InitializeObject(Overlay12AttackState *state);
void Overlay12Controller_StartLandingBounce(Overlay12AttackController *controller, int bounce);
void Overlay12Controller_BeginFinish(Overlay12AttackController *controller);
int Overlay12Controller_IsIdle(Overlay12AttackController *controller);
void Overlay12Controller_UpdateArcAnimation(Overlay12AttackController *controller);
void Overlay12Controller_ContinueArc(Overlay12AttackController *controller);
void func_ov012_020c3afc(Overlay12AttackController *controller);
void Overlay12Controller_ScheduleLaunch(Overlay12AttackController *controller, int value);
int Overlay12Controller_IsReady(Overlay12AttackController *controller);
void Overlay12Controller_Prepare(Overlay12AttackController *controller);
void Overlay12Attack_Initialize(BattlePartyActor *user);
void func_ov012_020c50c0(BattlePartyActor *user);
void func_ov012_020c5314(BattlePartyActor *user);
void Overlay12Motion_UpdateDampedArc(BattleSceneObject *object, BattleMotionChannel *channel);
}
#endif
