#ifndef PIT_GAME_OVERLAY017_PARTICIPANT_H
#define PIT_GAME_OVERLAY017_PARTICIPANT_H
#ifdef __cplusplus
extern "C" {
#endif
#include <nitro.h>
#ifdef __cplusplus
}
#endif
#include <game/battle_effect.h>
#include <game/overlay017_attack.h>
typedef struct Overlay17Participant Overlay17Participant;
typedef struct Overlay17ParticipantFlags {
    u16 phase : 5, active : 1, formation : 2, input : 2, index : 5, unknown15 : 1;
} Overlay17ParticipantFlags;
typedef char Overlay17ParticipantFlags_SizeCheck[sizeof(Overlay17ParticipantFlags) == 2 ? 1 : -1];

/* The native state base overlaps the four final bytes of BattleSceneObject. */
typedef struct Overlay17ParticipantState {
    u8 shared_object_tail[4];
    Overlay17Participant *next;
    int animation_component, tuning, counter, bounce, duration;
    BattleEffect *effect;
    u16 target;
    s16 height;
    union {
        u16 flags;
        Overlay17ParticipantFlags bits;
    };
    u16 unknown26;
} Overlay17ParticipantState;
struct Overlay17Participant {
    union {
        BattleSceneObject object;
        struct {
            u8 unknown000[256];
            Overlay17ParticipantState state;
        };
    };
};

typedef char Overlay17ParticipantState_SizeCheck[sizeof(Overlay17ParticipantState) == 40 ? 1 : -1];
typedef char Overlay17Participant_SizeCheck[sizeof(Overlay17Participant) == 296 ? 1 : -1];
#ifdef __cplusplus
extern "C" {
#endif
void Overlay17Participant_ConsumeInput(Overlay17Participant *participant);
void Overlay17Participant_ResolveHit(Overlay17Participant *participant);
int Overlay17Participant_FindIdleSlot(void);
void Overlay17Participant_RemoveIdleFromList(void);
void Overlay17Participant_BeginEntry(Overlay17Participant *participant, int formation);
void Overlay17Participant_ReleaseModel(Overlay17Participant *participant);
void Overlay17Participant_SetAnimation(Overlay17Participant *participant, int command, int value, int render_state);
int Overlay17Participant_GetTravelDuration(Overlay17Participant *participant, int height);
void Overlay17Participant_Initialize(Overlay17Participant *participant, int index);
void Overlay17AttackObject_InitializeGroup(Overlay17AttackObject *attack_object, int group);
void Overlay17Participant_AppendToList(Overlay17Participant *participant);
void Overlay17Participant_GetHomePosition(Overlay17Participant *participant, s16 *x, s16 *y);
Overlay17ParticipantState *Overlay17Participant_Hide(Overlay17Participant *participant);
void Overlay17Participant_BeginRemoval(Overlay17Participant *participant);
void Overlay17Participant_ForwardVariantControl(Overlay17Participant *participant, int adult_value, int baby_value);
void Overlay17Participant_ReturnOthers(Overlay17Participant *participant, int return_others);
void Overlay17Participant_ResumeArc(Overlay17Participant *participant);
void Overlay17Participant_BeginRecoveryAnimation(Overlay17Participant *participant);
void Overlay17Participant_BeginExit(Overlay17Participant *participant);
void Overlay17Participant_BeginExitArc(Overlay17Participant *participant);
void Overlay17AttackObject_Initialize(Overlay17AttackObject *attack_object, int group, int phase, int animation_component,
                                      int enabled);
void Overlay17AttackObject_ReleaseModel(Overlay17AttackObject *attack_object);
#ifdef __cplusplus
}
#endif
#endif
