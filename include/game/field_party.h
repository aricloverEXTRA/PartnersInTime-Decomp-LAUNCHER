#ifndef PIT_GAME_FIELD_PARTY_H
#define PIT_GAME_FIELD_PARTY_H

#include <game/field_entity.h>
#include <game/field_resources.h>
#include <game/field_party_trail.h>

/* Checked allocation layouts with unknown interiors retained as opaque fields.
 * The party entity extends FieldRuntimeEntity. The low flag bytes save collision state, while the
 * same word holds the movement mode used by the state dispatcher. */
typedef struct FieldPartyFollowerFlags { u16 enabled : 1, unknown_01_15 : 15; } FieldPartyFollowerFlags;
typedef char FieldPartyFollowerFlags_SizeCheck[sizeof(FieldPartyFollowerFlags) == 2 ? 1 : -1];
typedef struct FieldPartyEntity {
    FieldRuntimeEntity entity;
    union {
        u32 flags;
        struct {
            u32 unknown_00_15 : 16, saved_collision_faces : 1, collision_faces_saved : 1;
            u32 movement_mode : 4, unknown_22 : 1, unknown_23 : 1, unknown_24 : 1;
            u32 unknown_25_30 : 6, unknown_31 : 1;
        } bits;
        struct {
            u32 saved_00 : 1, saved_01 : 1, saved_02 : 1, saved_03 : 1, saved_04 : 1, saved_05 : 1, saved_06 : 1, saved_07 : 1;
            u32 has_00 : 1, has_01 : 1, has_02 : 1, has_03 : 1, has_04 : 1, has_05 : 1, has_06 : 1, has_07 : 1;
            u32 unknown_16_31 : 16;
        } collision_backup;
        /* The auxiliary renderer uses the same word for its own state. */
        struct {
            u32 unknown_00 : 1, unknown_01 : 1, direction_mode : 2, unknown_04 : 1;
            u32 unknown_05_06 : 2, unknown_07_08 : 2, unknown_09 : 1;
            u32 unknown_10_15 : 6, unknown_16_18 : 3, unknown_19 : 1;
            s32 unknown_20_27 : 8, unknown_28_31 : 4;
        } auxiliary_bits;
        struct {
            u8 saved_collision, collision_saved, unknown_02[2];
        } bytes;
    };
    union {
        FieldLinearController movement;
        struct {
            u32 unknown_524[6];
            fx32 unknown_53c, unknown_540, unknown_544;
            u8 unknown_548[4];
            struct FieldPartyEntity *target;
            u8 unknown_550[24];
        } auxiliary_motion;
    };
    union { u16 *unknown_568; FieldPartyFollowerFlags *follower_flags; };
    union { u8 unknown_56c[4]; u32 unknown_56c_word; struct FieldPartyEntity *following_target; };
    struct FieldPartyEntity *linked_member;
    struct FieldPartyEntity *partner;
    union {
        struct {
            struct FieldPartyEntity *auxiliary;
            u8 unknown_57c[20];
        };
        struct FieldAuxiliaryEntity *auxiliaries[6];
    };
    s16 saved_animation_speed;
    struct {
        u16 unknown_00 : 1, saved_behavior : 3, behavior_saved : 1, unknown_05 : 1;
        s16 resource_index : 8;
        u16 unknown_14 : 1, unknown_15 : 1;
    } presentation;
    struct { u8 unknown_00 : 1, unknown_01_07 : 7; } unknown_594;
    u8 unknown_595;
    s8 unknown_596, unknown_597, paired_bounds_index, previous_paired_bounds_index;
    u8 unknown_59a[2];
    union { u32 unknown_59c; struct FieldPartyStateRecord *state_record; };
} FieldPartyEntity;
typedef char FieldPartyEntity_SizeCheck[sizeof(FieldPartyEntity) == 0x5a0 ? 1 : -1];

typedef struct FieldPartySnapshot FieldPartySnapshot;
typedef struct FieldPartyAnchorOrbit {
    u8 unknown_00[20];
    u16 angle, angular_speed;
    u8 unknown_18[8];
} FieldPartyAnchorOrbit;
typedef char FieldPartyAnchorOrbit_SizeCheck[sizeof(FieldPartyAnchorOrbit) == 32 ? 1 : -1];

typedef struct FieldPartyStateRecord {
    union {
        u32 unknown_00[8];
        FieldPartyAnchorOrbit anchor_orbit;
        struct {
            struct { u32 unknown_00_10 : 11, copy_animation_flags : 1, unknown_12_31 : 20; } flags;
            u8 unknown_04[28];
        } auxiliary_animation;
        struct { u8 auxiliary_index, elapsed_frames, unknown_02[30]; } auxiliary_launch;
        struct {
            fx32 body_vertical_extent, navigation_vertical_extent;
            u8 unknown_08[24];
        } bounds_snapshot;
        struct {
            union { u32 unknown_00; fx32 saved_movement_speed; };
            struct {
                u32 unknown_00 : 1, unknown_01_14 : 14, saved_movement : 4, update_bounds : 1;
                u32 unknown_20_21 : 2, unknown_22_31 : 10;
            } flags;
            u8 unknown_08[8];
            u8 bob_phase, unknown_11[7];
            const void *resource_18, *resource_1c;
        } resources;
        struct {
            u32 unknown_00[2];
            struct { u32 unknown_00_14 : 15, area_value_saved : 1, unknown_16_31 : 16; } flags;
            u32 unknown_0c[3];
            u32 saved_area_value, unknown_1c;
        } member_action;
        struct {
            fx32 velocity, acceleration, target_z;
            u16 angle, angular_speed;
            union { s16 unknown_10; s16 angular_acceleration; };
            struct { u16 facing_direction : 3, unknown_03 : 1, unknown_04_15 : 12; } flags;
            s16 timer;
            u8 unknown_16[2];
            u16 linked_member_delay;
            u8 unknown_1a[6];
        };
    };
} FieldPartyStateRecord;
typedef char FieldPartyStateRecord_SizeCheck[sizeof(FieldPartyStateRecord) == 32 ? 1 : -1];

/* Partial view of the follower state; the height gate can restart its path. */
typedef struct FieldPartyFollowerHeightState {
    struct { u16 unknown_00 : 1, height_wait : 1, unknown_02 : 1, unknown_03_15 : 13; } flags;
    u8 unknown_02[26];
    int distance;
    u8 unknown_20[2176];
} FieldPartyFollowerHeightState;
typedef char FieldPartyFollowerHeightState_SizeCheck[sizeof(FieldPartyFollowerHeightState)==2208?1:-1];
typedef struct FieldPartyController {
    union {
        struct {
            u16 unknown_00 : 1, unknown_01 : 1, unknown_02_15 : 14;
        } state;
        struct {
            u16 members : 2, unknown_02_03 : 2, unknown_04_15 : 12;
        } state_groups;
    };
    u8 unknown_002, unknown_003;
    u16 input_masks[2];
    FieldPartyStateRecord records[2];
    union {
        struct {
            FieldPartyEntity *leader, *follower;
        };
        FieldPartyEntity *members[2];
    };
    struct {
        u32 unknown_00_01 : 2, unknown_02 : 1, unknown_03 : 1, unknown_04 : 1, unknown_05 : 1, unknown_06 : 1;
        u32 field_screen : 1, previous_field_screen : 1, special_contact_mode : 1;
        u32 unknown_10_13 : 4, unknown_14 : 1, follower_rejoin_active : 1;
        u32 unknown_16 : 1, active_member : 1, movement_active : 1;
        u32 unknown_19 : 1, movement_state : 10, palette_animation_active : 1, unknown_31 : 1;
    } flags;
    u8 unknown_054;
    s8 unknown_055;
    u8 unknown_056[2];
    u32 unknown_058, unknown_05c;
    fx32 separation_x, separation_y, separation_z;
    u32 unknown_06c;
    u8 actions[4];
    union {
        u32 state_flags;
        struct {
            u32 visible : 1, unknown_01_04 : 4, slide_from_bottom : 1;
            u32 visibility_transition : 1, unknown_07 : 1, unknown_08 : 1;
            u32 sliding : 1, swap_active : 1, unknown_11_31 : 21;
        } indicator_bits;
        struct {
            u32 unknown_00 : 1, unknown_01 : 1, unknown_02 : 1;
            u32 unknown_03_04 : 2, unknown_05 : 1, unknown_06_10 : 5, unknown_11_12 : 2;
            u32 unknown_13 : 1, unknown_14_17 : 4, movement_mode : 4, unknown_22_31 : 10;
        } state_bits;
    };
    union {
        u8 unknown_078[16];
        struct {
            union { u16 unknown_078_halfword; u16 indicator_frame; };
            s16 indicator_velocity, indicator_x, indicator_y;
            s16 indicator_offset_x, indicator_offset_y, indicator_slide_y;
            u16 unknown_086;
        };
    };
    FieldRenderObject *unknown_088, *unknown_08c, *unknown_090;
    union {
        u8 unknown_094[2212];
        struct { FieldRenderObject *unknown_094_render; u8 follower_state[2208]; };
        struct { u8 unknown_094_prefix[4]; FieldPartyFollowerHeightState follower_height; };
    };
    FieldResourceContext **areas;
    struct FieldPartyController *paired;
    u8 *resources;
    FieldPartyTrailHistory histories[2];
    FieldPartyTrailEmitter trail_emitter;
    struct {
        u32 active : 1, movement_state : 10, unknown_11 : 1, snapshot_backed : 1, unknown_13_31 : 19;
    } backup;
    FieldPartyEntity backup_entities[2];
    const FieldPartySnapshot *backup_snapshots[2];
} FieldPartyController;
typedef char FieldPartyController_SizeCheck[sizeof(FieldPartyController) == 0x20a4 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
void FieldPartyEntity_MapLocomotionState(FieldPartyEntity *member);
void FieldPartyEntity_UpdateLocomotionState(FieldPartyEntity *member);
fx32 FieldPartyEntity_GetDirectionVector(FieldPartyEntity *member, int direction, fx32 *x, fx32 *y);
void FieldParty_UpdateAuxiliaryAnimationTransition(FieldPartyController *party, FieldPartyEntity *member, int immediate);
fx32 FieldParty_GetFollowerOffset(FieldPartyController *party, FieldPartyEntity *member, fx32 distance, fx32 *x, fx32 *y);
void FieldParty_ResetDefaultActions(FieldPartyController *party);
void FieldParty_UpdateSeparation(FieldPartyController *party);
void FieldPartyEntity_RestoreContactFlags(FieldPartyEntity *member);
void FieldParty_UpdateFollowerHeightGate(FieldPartyController *party);
void FieldParty_BindFollowerState(FieldPartyEntity *member, void *state, FieldPartyEntity *target);
fx32 FieldParty_GetSlopeDistanceScale(FieldPartyEntity *member, int direction,
                                     const FieldNavigationSurface *surface);
void FieldParty_ClearLeaderPaletteAnimation(FieldPartyController *party, int force);
void FieldParty_StartLeaderPaletteAnimation(FieldPartyController *party, int force);
void FieldPartyEntity_AdvanceFastSpin(FieldPartyEntity *member);
void FieldPartyEntity_AdvanceSlowSpin(FieldPartyEntity *member);
void FieldPartyEntity_SyncPartnerPosition(FieldPartyEntity *member);
FieldPartyEntity *FieldPartyEntity_InitPlacement(FieldPartyEntity *party, int index,
    const FieldSpawnRecord *spawn, int screen, int resource_set, u8 enabled, int baby);
void FieldParty_UpdateRegion71ActionModel(FieldPartyController *party, int member);
void FieldParty_InitializeIndicator(FieldPartyController *party, int visible, int screen, int layout, int style, int mode);
void FieldParty_ResetActionState(FieldPartyController *party, int immediate, int preserve_state);
void FieldParty_BeginEntrySlide(FieldPartyController *party);
void FieldParty_AlignMovementToFacing(FieldPartyController *party);
void FieldParty_CopyAttachedRenderPriorities(FieldPartyController *party);
void FieldParty_BindActionResource(FieldPartyController *party, FieldPartyEntity *member, int index,
                                  int speed, u8 restart);
void FieldParty_BeginAuxiliaryPlacement(FieldPartyController *party);
void FieldParty_SetAttachedSpriteVisible(FieldPartyController *party, int visible);
void FieldParty_FinishState76(FieldPartyController *party);
void FieldParty_FinishAuxiliaryLaunch(FieldPartyController *party);
void FieldParty_LaunchAuxiliary(FieldPartyController *party);
void FieldParty_LaunchToElevation(FieldPartyController *party, fx32 height);
void FieldParty_BeginState89(FieldPartyController *party);
void FieldParty_BeginBabyDrop(FieldPartyController *party);
void FieldParty_LandBabyDrop(FieldPartyController *party,int member);
void FieldParty_RestoreMode5(FieldPartyController *party,int preserve_state);
void FieldParty_BeginState76(FieldPartyController *party);
void FieldParty_BeginState78(FieldPartyController *party);
void FieldParty_BeginState68(FieldPartyController *party, int member);
void FieldParty_ResumeState62Or74(FieldPartyController *party, int preserve_state);
void FieldParty_LandBabySpin(FieldPartyController *party, int member);
void FieldParty_CancelBabySpin(FieldPartyController *party);
void FieldParty_GlideBabySpin(FieldPartyController *party);
void FieldParty_HoldBabySpin(FieldPartyController *party);
void FieldParty_UpdateBabySpinAnchorOrbit(FieldPartyController *party, FieldPartyEntity *member);
void FieldParty_InitializeBabySpinAnchorOrbit(FieldPartyController *party, FieldPartyEntity *member);
void FieldParty_TryAnchorBabySpin(FieldPartyController *party, int index);
void FieldParty_BeginBabySpin(FieldPartyController *party, int member);
void FieldParty_BeginHammerSwing(FieldPartyController *party, int member);
void FieldParty_EndHammerSwing(FieldPartyController *party, int member);
int FieldParty_IsHammerActive(FieldPartyController *party, int member);
void FieldParty_BeginBrosBallState23(FieldPartyController *party, int direction);
void FieldParty_RestoreBrosBallRoll(FieldPartyController *party);
void FieldParty_ResumeBrosBall(FieldPartyController *party, int preserve_state);
void FieldParty_ExitBrosBall(FieldPartyController *party);
void FieldParty_StopBrosBallRoll(FieldPartyController *party);
void FieldParty_StartBrosBallRoll(FieldPartyController *party, int fast);
void FieldParty_BeginBrosBall(FieldPartyController *party);
void FieldParty_TryFollowEntityAtHeight(FieldPartyController *party, int index);
void FieldParty_StackFollower(FieldPartyController *party);
void FieldParty_RestoreGroundMovementAndHideFollower(FieldPartyController *party);
void FieldParty_SeparateFollower(FieldPartyController *party);
void FieldParty_MoveFollowerToSeparationOffset(FieldPartyController *party);
void FieldParty_ResumeGroundMovement(FieldPartyController *party, int preserve_state);
void FieldParty_BeginSpinJump(FieldPartyController *party);
void FieldParty_BeginState11(FieldPartyController *party);
void FieldParty_BeginSpinJumpFall(FieldPartyController *party, u8 mode);
void FieldParty_RestoreGroundMovement(FieldPartyController *party);
void FieldParty_ResumeState11Or17(FieldPartyController *party, int preserve_state);
void FieldParty_DisableFollowing(FieldPartyController *party);
void FieldParty_ResetFollowingContacts(FieldPartyController *party);
void FieldParty_InitializeFollowing(FieldPartyController *party, int argument);
void FieldParty_UpdateFollowing(FieldPartyController *party);
void FieldParty_EnableFollowing(FieldPartyController *party);
int FieldParty_CheckFollowerRejoin(FieldPartyController *party);
void FieldParty_RejoinFollower(FieldPartyController *party, int instant);
void FieldParty_ResetActionSelection(FieldPartyController *party);
void FieldParty_RefreshActions(FieldPartyController *party);
void FieldParty_SetMemberAction(FieldPartyController *party, int member, int action);
void FieldParty_ToggleAvailableActions(FieldPartyController *party, int pressed);
void FieldParty_ProcessInput(FieldPartyController *party, int held, int pressed, int change_actions);
void FieldParty_UpdateIndicatorVisibility(FieldPartyController *party);
void FieldParty_SetIndicatorVisibility(FieldPartyController *party, int visible, int instant, int transfer);
void FieldParty_UpdateIndicatorSwap(FieldPartyController *party);
void FieldParty_StartIndicatorSwap(FieldPartyController *party);
void FieldParty_SetIndicatorLayer(FieldPartyController *party, int alternate);
void FieldParty_UpdateIndicatorPositions(FieldPartyController *party);
void FieldParty_UpdateIndicatorSlide(FieldPartyController *party);
void FieldParty_StartIndicatorSlide(FieldPartyController *party, int upward);
void FieldPartyEntity_BeginFrame(FieldPartyEntity *party);
void FieldPartyEntity_SetNavigationSurfaces(FieldPartyEntity *party, FieldNavigationSurface *surfaces,
                                           const void *resource);
void FieldPartyEntity_ReleaseRenderers(FieldPartyEntity *party);
void FieldPartyEntity_AllocateRenderers(FieldPartyEntity *party);
FieldPartyEntity *FieldPartyEntity_InitEmpty(FieldPartyEntity *party);
FieldPartyEntity *FieldPartyEntity_Destroy(FieldPartyEntity *party);
FieldPartyEntity *FieldPartyEntity_Delete(FieldPartyEntity *party);
FieldPartyEntity *FieldPartyEntity_CopyState(FieldPartyEntity *entity, const FieldPartyEntity *source);
FieldPartyEntity *FieldPartyAuxiliary_CopyState(FieldPartyEntity *entity, const FieldPartyEntity *source);
void FieldParty_LinkPairedMember(FieldPartyController *party, int member);
void FieldParty_StartPairedFollowing(FieldPartyController *party);
void FieldParty_WaitForPairedFollowing(FieldPartyController *party);
void FieldParty_PrepareCollisionOverride(FieldPartyController *party, int preserve_state);
void FieldParty_RestoreCollisionState(FieldPartyController *party, int check_contacts);
void FieldParty_RestartCollisionOverride(FieldPartyController *party);
void FieldParty_InitializeAuxiliary(FieldPartyController *party, int preserve_state);
void FieldParty_UpdateAuxiliaryFacing(FieldPartyController *party);
void FieldParty_ReverseFollowerOffset(FieldPartyController *party);
void FieldParty_ReturnFollowerToLeader(FieldPartyController *party);
void FieldParty_RestartAuxiliary(FieldPartyController *party);
void FieldParty_RaiseFollowerToLeader(FieldPartyController *party);
/* State 34's gameplay identity has not yet been observed. */
void FieldParty_BeginState34(FieldPartyController *party);

struct FieldPairedBoundsRuntime;
int FieldPartyEntity_FindPairedBounds(FieldPartyEntity *member, const struct FieldPairedBoundsRuntime *records);
int FieldPartyEntity_HasActiveAction(FieldPartyEntity *member);
void FieldPartyEntity_RestoreRenderSnapshot(FieldPartyEntity *member, const FieldRenderSnapshot *snapshot);
void FieldPartyEntity_SaveRenderSnapshot(FieldPartyEntity *member, FieldRenderSnapshot *snapshot);
void FieldPartyEntity_BindStateResource1C(FieldPartyEntity *member);
void FieldPartyEntity_BindStateResource18(FieldPartyEntity *member);
void FieldPartyEntity_HideBlinkRenderers(FieldEntity *entity);
void FieldPartyEntity_ShowBlinkRenderersWithSound(FieldEntity *entity);
void FieldPartyEntity_CopySortKeysToAnchors(FieldPartyEntity *member);
void FieldPartyEntity_UpdateAuxiliaryPriorities(FieldPartyEntity *member);
void FieldPartyEntity_UpdateAnimation(FieldPartyEntity *member, int mode, u8 update_bounds);
void FieldPartyEntity_UpdateScreenPositions(FieldPartyEntity *member, s16 camera_x, s16 camera_y);
void FieldPartyEntity_UpdateRenderers(FieldPartyEntity *member, u8 default_priority);
void FieldPartyEntity_CopyPartnerPlanarBounds(FieldPartyEntity *member);
void FieldPartyEntity_SetCollisionResponseChannelsMasked(FieldPartyEntity *member, u8 mask, u8 enabled);
void FieldPartyEntity_SetCollisionResponseChannels(FieldPartyEntity *member, int first, int second, int third,
                                                 int fourth, int fifth);
/* Type-3 queries return zero for an overlapping volume selected by bits 0/3 or 0/2. */
int FieldPartyEntity_TestType3Volumes03(FieldPartyEntity *member);
int FieldPartyEntity_TestType3Volumes02(FieldPartyEntity *member);

void FieldParty_RestorePairedMember(FieldPartyController *party,int member);
void FieldParty_BeginPiggybackMount(FieldPartyController *party,int member);
void FieldParty_FinishAirborneTransfer(FieldPartyController *party,int member);

void FieldParty_FinishRecoveryBlink(FieldPartyController *party, int member);
void FieldParty_BeginRecoveryBlink(FieldPartyController *party, int member);
void FieldParty_UpdateHover(FieldPartyController *party, int member);
void FieldParty_BeginState8(FieldPartyController *party);
void FieldParty_BeginPairedStates82Or83(FieldPartyController *party, int member_mask, int airborne);
void FieldParty_BeginState81(FieldPartyController *party, int airborne);
void FieldParty_BeginAirborneTransfer(FieldPartyController *party, int member);

#ifdef __cplusplus
}
#endif
#endif
