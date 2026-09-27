#ifndef PIT_OVERLAY015_ACTOR_INTERNAL_H
#define PIT_OVERLAY015_ACTOR_INTERNAL_H

#include <game/overlay015_attack.h>
#include <game/battle_object.h>
#include <game/battle_feedback.h>
#include <game/battle_context.h>
#include <game/overlay010_motion.h>

/* The native attack constructor allocates 584 bytes. */
typedef struct Overlay15AttackContext {
    s32 timer;
    u8 unknown_004[12];
    union {
        u8 flags;
        struct {
            u8 unknown_0 : 1;
            u8 finished : 1;
            u8 unknown_2_7 : 6;
        } flag_bits;
    };
    s8 phase;
    u8 unknown_012[22];
    BattlePartyActor *user;
    u8 unknown_02c[160];
    s32 index;
    u8 unknown_0d0[52];
    s32 power;
    u8 unknown_108[40];
    Overlay15AttackModelController actors[2];
    Overlay15AttackObjectPairState pair;
    u8 unknown_1bc[4];
    const Overlay15AttackRewardItemPrefix *item;
    s16 message_handle;
    s16 message_width;
    u8 message[128];
} Overlay15AttackContext;

typedef char Overlay15AttackContext_SizeCheck[
    sizeof(Overlay15AttackContext) == 584 ? 1 : -1];
typedef char Overlay15AttackContext_PowerOffsetCheck[
    (u32)&((Overlay15AttackContext *)0)->power == 260 ? 1 : -1];
typedef char Overlay15AttackContext_MessageWidthOffsetCheck[
    (u32)&((Overlay15AttackContext *)0)->message_width == 454 ? 1 : -1];
typedef char Overlay15AttackContext_MessageOffsetCheck[
    (u32)&((Overlay15AttackContext *)0)->message == 456 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
#include <game/random.h>
extern Overlay15AttackContext *data_ov002_020c0710;

struct Overlay10ActionActor;
void Overlay15Attack_UpdateEntry(struct Overlay10ActionActor *actor);
void Overlay15Attack_BeginActorFinish(Overlay15AttackModelController *state);
void Overlay15Attack_ResolvePairHit(Overlay15AttackModelController *state,
    Overlay15AttackModelController *other, Overlay15AttackObjectPairState *pair);
void func_ov015_020c31dc(Overlay15AttackObjectPairState *pair);
void Overlay15Attack_UpdateSequence(BattlePartyActor *actor);
void BattleAttack_SetCallback(BattlePartyActor *actor,
    void (*callback)(BattlePartyActor *));

/* Interleaved X/Y offsets; each actor's entry occupies two bytes. */
extern s8 data_ov015_020c5e60[][2];
extern s8 data_ov015_020c5e61[][2];
extern s8 data_ov015_020c5e68[][2];
extern s8 data_ov015_020c5e69[][2];

/* Formation-specific parameters have a six-byte record stride. */
extern s16 data_ov015_020c5f1c[][3];
extern s16 data_ov015_020c5ef8[][3];
extern s16 data_ov015_020c5f40[][3];
extern u16 data_ov015_020c5fdc[][2];
extern u16 data_ov015_020c5fde[][2];
extern u16 data_ov015_020c6054[][2];
extern u16 data_ov015_020c5fa0[];

int Overlay10Enemy_IsSelectable(u16 actor_id);

u16 func_02009224(BattleModel *model, s16 animation_id);
void func_0200940c(BattleModel *model, s16 speed_q8);
int BattleMotion_StartFollower(BattleSceneObject *object,
    BattleSceneObject *follower, int channel);
void BattleMotion_StartAppear(BattleSceneObject *object, int channel, int duration);
#ifdef __cplusplus
}
#endif

#endif
