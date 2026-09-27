#ifndef PARTNERS_IN_TIME_OVERLAY017_ATTACK_STATE_H
#define PARTNERS_IN_TIME_OVERLAY017_ATTACK_STATE_H

/*
 * The Copy Flower attack's state.
 */

#include <nitro.h>
#include <game/battle_scene.h>
#include <game/overlay017_participant.h>

enum Overlay17BattleStateConstant {
    OVERLAY17_ATTACK_STATE_COUNT = 8,
    OVERLAY17_ATTACK_STATE_SIZE = 0x128,
    OVERLAY17_PARTY_ANIMATION_COMPONENT_COUNT = 2,
};

typedef struct Overlay17BattleStateView {
    u8 unknown_000[0xCC];
    int successful_hits;
    u8 unknown_0d0[0x32];
    u16 target;
    int power[2];
    u8 unknown_10c[0x24];
    u8 attack_states[OVERLAY17_ATTACK_STATE_COUNT][OVERLAY17_ATTACK_STATE_SIZE];
    Overlay17Participant *last_participant, *head;
    int tuning, spawn_timer;
    u16 input_mask, pressed_mask;
    union {
        u8 setup_flags;
        struct { u8 random_target : 1, unknown_1_7 : 7; } setup_bits;
    };
    u8 control_flags;
    s8 active_attack_slot;
    u8 unknown_a87;
    int party_animation_components[OVERLAY17_PARTY_ANIMATION_COMPONENT_COUNT];
    u8 unknown_a90[8];
    struct BattlePartyActor *party[4];
} Overlay17BattleStateView;

typedef struct Overlay17AttackObject {
    BattleSceneObject object;
    int animation_component;
    int motion_variant;
    u8 unknown_10c[0x40];
    s16 scale_x, scale_y;
    union {
        u32 flags;
        struct {
            u32 state : 4;
            u32 resource_variant : 3;
            u32 animation_phase : 3;
            u32 unknown_10_13 : 4;
            u32 unknown_14 : 1;
            u32 enabled : 1;
            u32 unknown_16_31 : 16;
        } flag_bits;
    };
} Overlay17AttackObject;

/* Recovered prefix; the complete attack allocation is larger. */
typedef char Overlay17BattleStateView_SizeCheck[sizeof(Overlay17BattleStateView) == 0xAA8 ? 1 : -1];
typedef char Overlay17AttackObject_SizeCheck[sizeof(Overlay17AttackObject) == 0x154 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif

extern Overlay17BattleStateView *data_ov002_020c0710;

#ifdef __cplusplus
}
#endif

#endif
