/*
 * Field script dispatcher (overlay 0, 0x020823F8-0x02087FBC).
 *
 * The opcode table of the overworld's scripting language: everything a room
 * script can do - move the party, run a cutscene, open a window, change the
 * music, award an item, switch rooms - is a case in one switch here.
 *
 * The file is deliberately one large function. Splitting the cases into separate
 * handlers would read better but would change the register allocation and the
 * literal pools the original compiler produced for this switch, so the shape is
 * part of the match. See docs/research/SCRIPT_VM_SEMANTICS.md for what the
 * opcodes mean and how their operands are encoded.
 */

#include <game/field_party_manager.h>
#include <game/field_scene_transition.h>
#include <game/field_resources.h>
#include <game/field_effect_animation.h>
#include <game/field_room_interaction.h>
#include <game/field_room_transition.h>
#include <game/field_roaming.h>
#include <game/field_party.h>
#include <game/field_party_manager.h>
#include <game/field_system.h>
#include <game/field_transform.h>
#include <game/field_entity_motion.h>
#include <game/field_linear.h>
#include <game/field_orbit.h>
extern "C" {
#include <game/field_variable_entity.h>
#include <game/field_area_motion.h>
#include <game/field_entity.h>
#include <game/field_presentation.h>
#include <game/field_entity_lifecycle.h>
#include <game/field_timer.h>
#include <game/field_palette_crossfade.h>
#include <game/field_hud.h>
#include <game/field_timed_renderer.h>
#include <game/field_script.h>
#include <game/field_script_manager.h>
#include <game/save_data.h>

/*
 * Structured reconstruction of the field-specific VM command range. The
 * original is a single 23,492-byte switch. The C++ virtual interfaces retain
 * its C entry-point ABI and reproduce every byte in the European ROM.
 */

extern void func_ov000_020a4214(FieldEntity *entity, int angle_mode,
                                int target_angle, int angular_step,
                                int signed_multiplier, int stop_at_target,
                                int reserved);
extern void func_ov000_020b2e9c(
    FieldEntity *entity, int relative_center, fx32 center_x, fx32 center_y,
    fx32 center_z, int relative_angle, int angle, int initial_speed,
    int acceleration, int maximum_speed, int deceleration, int plane,
    int direction, int secondary_axis_scale, int stop_on_contact_mask,
    int stop_on_state_mask, int snap_to_final_angle, int reserved);
extern void func_ov000_020b295c(
    FieldEntity *entity, FieldEntity *center_entity, fx32 center_x,
    fx32 center_y, fx32 center_z, int relative_angle, int angle, int speed,
    int plane, int direction, int secondary_axis_scale,
    int stop_on_contact_mask, int stop_on_state_mask,
    int snap_to_final_angle, int reserved);
extern void func_ov000_020b26ac(
    FieldEntity *entity, FieldEntity *center_entity, fx32 center_x,
    fx32 center_y, fx32 center_z, int relative_angle, int angle,
    int duration, int plane, int direction, int secondary_axis_scale,
    int stop_on_contact_mask, int stop_on_state_mask,
    int snap_to_final_angle, int reserved);
extern void func_ov000_020b1a24(FieldEntity *entity, const void *path,
                                int path_size_halfwords);
extern int func_ov000_0207133c(u8 *field_context, FieldEntity *entity);
extern void func_ov000_02071a38(u8 *field_context, int layout_mode,
                                int instant);
extern void func_ov000_020a0c30(void *party_manager, int party_side,
                                int instant, int reserved, int enabled);
extern void func_ov000_0209f644(
    void *party_manager, int party_side, int destination_room_id, int x,
    int y, int z, int facing_direction, int animate_entry,
    s16 arrival_script_id, int bgm_policy, int preserve_field_0_anchor,
    int preserve_field_1_anchor);
extern void func_ov000_02075814(
    u8 *field_context, int animation_slot, int direction_profile,
    u16 object_mask_high, u16 object_mask_low, u16 standard_bg_mask,
    u16 extended_bg_mask_0, u16 extended_bg_mask_1,
    u16 extended_bg_mask_2, s16 duration, u16 color);

extern void func_ov000_02074810(u8 *field_context, int wipe_type,
                                const void *parameters);
extern void func_02036988(volatile u32 *blend_control,
                          int first_target_plane_mask,
                          int second_target_plane_mask,
                          int first_coefficient, int second_coefficient);
extern void func_ov000_02069d38(
    void *field_system, const s32 *entity_selectors, int anchor_entity,
    int destination_room_id, int arrival_script_id,
    int paired_arrival_script_id, int legacy_parameter,
    int travel_direction, int transit_room_id);

extern void func_ov000_02069284(
    void *field_system, const s32 *entity_selectors, int anchor_entity,
    int destination_room_id, int argument_4, int argument_5,
    int orbit_entities, s16 center_x_offset, s16 center_y_offset);
extern void func_020052b0(int scene_id, int argument_1);
extern void func_ov000_02079d74(u8 *field_context, int party_mode);
extern void func_ov000_0206ba2c(
    void *field_system, int start_mode, s16 encounter_id,
    int reserved_transition_argument, u16 source_room_id, int contact_mode,
    int field_screen, FieldEntity *encounter_entity,
    FieldEntity *primary_entity, int party_context,
    int transition_variant);
extern void GameParty_Initialize(int new_game_preset);
extern int GameInventory_Add(u16 item_id, int count_delta);
extern const u16 data_02048f1a[];
extern void GameRumble_PlayTimed(int rumble_pattern, int repeat_count);
extern u8 data_0205a00c;
extern void GameRumble_Stop(void);
extern int func_ov000_02070930(
    u8 *field_context, int window_mode, int x, int y, int width,
    int height, int tail_style, int tail_size, int vertical_placement,
    int reserved, int tail_x, int text_control_enabled,
    int text_archive_id, int message_id, int message_slot,
    int message_speed, u8 *message_state);
extern int func_ov000_020700ec(
    u8 *field_context, int window_mode, FieldEntity *entity, int width,
    int height, int tail_style, int tail_size, int vertical_placement,
    int tail_x, int text_control_enabled, int text_archive_id,
    int message_id, int message_slot, int message_speed,
    u8 *message_state, FieldEntity *linked_owner);
extern void func_ov000_0206f378(u8 *field_context, int window_slot,
                                fx32 x, fx32 y, int duration);
extern void GameAudio_PlayEffectDelayed(s16 sound_id, int playback_mode,
                          int reserved_channel);
extern void GameAudio_StopEffect(s16 sound_id);
extern void GameAudio_PlayVoice(s16 sequence_id, int reserved_channel);
extern void GameAudio_StopVoice(s16 sequence_id);
extern u32 GameAudio_GetMusicBanks(void);
/* Resident symbol names are retained here for the field audio commands. */
extern void GameAudio_LoadMusic(int sequence_id, u8 buffer_index,
                          int fade_duration);
extern int GameAudio_IsLoading(void);
extern void GameAudio_SetMusic(int buffer_index);
extern void GameAudio_StopMusic(int fade_duration);
extern void GameAudio_FadeMusic(int muted, int reserved_channel);
extern void GameAudio_StartStream(int sound_group_id);
extern int GameAudio_IsStreamPlaying(void);
extern void GameAudio_StopStream(void);
}

struct FieldMapController {
    virtual void unknown_00();
    virtual void unknown_04();
    virtual void unknown_08();
    virtual void unknown_0c();
    virtual void unknown_10();
    virtual void unknown_14();
    virtual void unknown_18();
    virtual void unknown_1c();
    virtual void unknown_20();
    virtual void unknown_24();
    virtual void unknown_28();
    virtual void unknown_2c();
    virtual void unknown_30();
    virtual void unknown_34();
    virtual void unknown_38();
    virtual void unknown_3c();
    virtual void unknown_40();
    virtual void start_tile_animation(u16 animation_index);
    virtual void unknown_48();
    virtual void pause_tile_animation(u16 animation_index);
    virtual void resume_tile_animation(u16 animation_index);
    virtual void unknown_54();
    virtual void unknown_58();
    virtual int are_palette_animations_active();
    virtual void unknown_60();
    virtual void start_palette_animation(u8 animation_index, u8 loop_mode);
    virtual void unknown_68();
    virtual void reset_palette_animations();
    virtual void stop_palette_animation(u8 animation_index);
    virtual void resume_palette_animation(u8 animation_index);
    virtual void restore_base_palettes();
};

typedef struct FieldCircularWipeParameters {
    u16 duration : 15;
    u16 retain_after_completion : 1;
    u8 center_x;
    u8 center_y;
    s16 start_radius;
    s16 end_radius;
    /* The transition engine consumes a fixed-size geometry parameter block. */
    u8 unused_08[0x0C];
} FieldCircularWipeParameters;

typedef struct FieldRectangularWipeParameters {
    u16 duration : 15;
    u16 retain_after_completion : 1;
    u8 center_x;
    u8 center_y;
    s16 start_left;
    s16 start_right;
    s16 start_top;
    s16 start_bottom;
    s16 end_left;
    s16 end_right;
    s16 end_top;
    s16 end_bottom;
} FieldRectangularWipeParameters;

typedef struct FieldInputChannelState {
    u8 unknown_000[0xC8];
    u16 overrides[4];
    u16 exclusion_masks[4];
} FieldInputChannelState;

typedef struct FieldSpecialResourceState {
    u16 unknown_00_04 : 5;
    u16 use_primary_resource_set : 1;
    s16 resource_index : 8;
    u16 unknown_14_15 : 2;
} FieldSpecialResourceState;

typedef struct FieldBrightnessTransitionFlags {
    u8 active : 1;
    u8 paused : 1;
    u8 unknown_02_07 : 6;
} FieldBrightnessTransitionFlags;

typedef struct FieldTwoBitActivityFlags {
    u16 activity_mode : 2;
    u16 unknown_02_15 : 14;
} FieldTwoBitActivityFlags;

typedef struct FieldSingleActivityFlags16 {
    u16 active : 1;
    u16 unknown_01_15 : 15;
} FieldSingleActivityFlags16;

typedef struct FieldAsyncOperationFlags {
    u32 active : 1;
    u32 unknown_01_31 : 31;
} FieldAsyncOperationFlags;

typedef struct FieldControlFlags {
    u16 unknown_00_02 : 3;
    u16 release_party_action : 1;
    u16 selected_party_side : 1;
    u16 unknown_05_15 : 11;
} FieldControlFlags;

typedef struct FieldBgScrollControllerState {
    FieldTwoBitActivityFlags flags;
    u8 unknown_02[54];
} FieldBgScrollControllerState;

typedef struct FieldSpecialPartyState {
    u16 field_screen : 1;
    u16 party_control_enabled : 1;
    u16 present_party_mask : 2;
    u16 unknown_04_13 : 10;
    u16 single_pass_collision : 1;
    u16 camera_focus_enabled : 1;
    s8 camera_focus_entity_id;
} FieldSpecialPartyState;

typedef struct FieldPartyControllerFlags {
    u32 unknown_00_06 : 7;
    u32 field_screen : 1;
    u32 unknown_08 : 1;
    u32 special_contact_mode : 1;
    u32 unknown_10_14 : 5;
    u32 follower_rejoin_active : 1;
    u32 unknown_16 : 1;
    u32 active_member : 1;
    u32 movement_active : 1;
    u32 unknown_19 : 1;
    u32 movement_state : 10;
    u32 unknown_30_31 : 2;
} FieldPartyControllerFlags;

typedef struct FieldPartyManagerFlags {
    u32 active_party_side : 1;
    u32 active_party_switching : 1;
    u32 unknown_02_06 : 5;
    u32 piggyback_state : 1;
    u32 unknown_08_16 : 9;
    u32 active_music_buffer : 7;
    u32 music_transition_level : 7;
    u32 secondary_music_resource_pending : 1;
} FieldPartyManagerFlags;

typedef struct FieldMenuPartyController {
    u8 unknown_0000[0x48];
    u8 *leader_entity;
    u8 unknown_004c[4];
    FieldPartyControllerFlags flags;
    u8 unknown_0054[0x2050];
} FieldMenuPartyController;

typedef struct FieldMenuPartyManager {
    u8 unknown_00[4];
    FieldPartyManagerFlags flags;
    u8 unknown_08[0x18];
    FieldMenuPartyController controllers[2];
} FieldMenuPartyManager;

typedef char FieldMenuPartyController_SizeCheck[
    sizeof(FieldMenuPartyController) == 0x20A4 ? 1 : -1
];

typedef struct FieldPartyManagerSoundState {
    u8 unknown_00[12];
    u16 tracked_sounds[4];
} FieldPartyManagerSoundState;

typedef struct FieldPartyControllerAuxFlags {
    u16 unknown_00 : 1;
    u16 secondary_action_active : 1;
    u16 unknown_02_15 : 14;
} FieldPartyControllerAuxFlags;

typedef struct FieldEntityAnimationRuntimeFlags {
    u32 alternate_variant : 1;
    u32 unknown_01_31 : 31;
} FieldEntityAnimationRuntimeFlags;

extern "C" void func_0200940c(FieldRenderObject *render_object, int speed);
extern "C" s32 FX_Atan2(fx32 y, fx32 x);

enum FieldVmOpcode {
    FIELD_VM_BRANCH_IF_OWNER_SUBTYPE = 0x033,
    FIELD_VM_BRANCH_RELATIVE = 0x034,
    FIELD_VM_SET_AUX_SCRIPT_ENABLED = 0x035,
    FIELD_VM_START_AUX_SCRIPT = 0x036,
    FIELD_VM_WAIT_AUX_SCRIPT = 0x037,
    FIELD_VM_STOP_AUX_SCRIPT = 0x038,
    FIELD_VM_PAUSE_AUX_SCRIPT = 0x039,
    FIELD_VM_RESUME_AUX_SCRIPT = 0x03A,
    FIELD_VM_GET_AUX_SCRIPT_STATE = 0x03B,
    FIELD_VM_START_INLINE_ENTITY_SCRIPT = 0x03C,
    FIELD_VM_START_INLINE_ENTITY_SCRIPT_AND_WAIT = 0x03D,
    FIELD_VM_START_RELATIVE_ENTITY_SCRIPT = 0x03E,
    FIELD_VM_WAIT_ENTITY_SCRIPT = 0x03F,
    FIELD_VM_WAIT_MATCHING_ENTITY_SCRIPTS = 0x040,
    FIELD_VM_STOP_ENTITY_SCRIPT = 0x041,
    FIELD_VM_STOP_MATCHING_ENTITY_SCRIPTS = 0x042,
    FIELD_VM_PAUSE_ENTITY_SCRIPT = 0x043,
    FIELD_VM_PAUSE_MATCHING_ENTITY_SCRIPTS = 0x044,
    FIELD_VM_RESUME_ENTITY_SCRIPT = 0x045,
    FIELD_VM_RESUME_MATCHING_ENTITY_SCRIPTS = 0x046,
    FIELD_VM_GET_ENTITY_SCRIPT_STATE = 0x047,
    FIELD_VM_START_PAIRED_FIELD_SCRIPT = 0x048,
    FIELD_VM_GET_ENTITY_PROPERTY = 0x049,
    FIELD_VM_SET_ENTITY_VISIBLE = 0x04A,
    FIELD_VM_SET_ENTITY_ENABLED = 0x04B,
    FIELD_VM_SET_ENTITY_TURN_TO_INTERACTOR_ENABLED = 0x04C,
    FIELD_VM_SET_ENTITY_GROUND_TRACKING = 0x04D,
    FIELD_VM_SET_ENTITY_ALTERNATE_COLLISION_FACES_ENABLED = 0x04E,
    FIELD_VM_SET_ENTITY_NAVIGATION_OBSTACLE_IGNORED = 0x04F,
    FIELD_VM_SET_ENTITY_OFFSCREEN_CONTACT_RETENTION_ENABLED = 0x050,
    FIELD_VM_SET_ENTITY_RESERVED_STATE_FLAG = 0x051,
    FIELD_VM_SET_ENTITY_LINKED_CONTACT_MODE = 0x052,
    FIELD_VM_SET_ENTITY_CONTACT_DIRECTION_FILTER = 0x053,
    FIELD_VM_SET_ENTITY_SEMITRANSPARENT = 0x054,
    FIELD_VM_CONFIGURE_ENTITY_SHADOW = 0x055,
    FIELD_VM_SET_ENTITY_MAP_SYNC_AXES = 0x056,
    FIELD_VM_SET_ENTITY_NAVIGATION_COLLISION_BOUNDS = 0x057,
    FIELD_VM_RESTORE_ENTITY_NAVIGATION_COLLISION_BOUNDS = 0x058,
    FIELD_VM_SET_ENTITY_COLLISION_RESPONSE_CHANNELS = 0x059,
    FIELD_VM_SET_ENTITY_COLLISION_RESPONSE_CHANNELS_MASKED = 0x05A,
    FIELD_VM_RESTORE_ENTITY_COLLISION_RESPONSE_CHANNELS = 0x05B,
    FIELD_VM_SET_ENTITY_BODY_COLLISION_BOUNDS = 0x05C,
    FIELD_VM_RESTORE_ENTITY_BODY_COLLISION_BOUNDS = 0x05D,
    FIELD_VM_SET_ENTITY_RESERVED_COLLISION_FLAG = 0x05E,
    FIELD_VM_SET_ENTITY_CATEGORY_COLLISION_POLICY = 0x05F,
    FIELD_VM_SET_ENTITY_RENDER_LAYER = 0x060,
    FIELD_VM_LEGACY_NOOP_061 = 0x061,
    FIELD_VM_SET_ENTITY_RENDER_ORDER_PRIORITIES = 0x062,
    FIELD_VM_SET_ENTITY_INTERACTION_BOUNDS = 0x063,
    FIELD_VM_LEGACY_NOOP_064 = 0x064,
    FIELD_VM_RESTORE_ENTITY_INTERACTION_BOUNDS = 0x065,
    FIELD_VM_SET_ENTITY_ANIMATION_SPEED = 0x066,
    FIELD_VM_SET_ENTITY_LOCOMOTION_PARAMETERS = 0x067,
    FIELD_VM_SET_ENTITY_DEFAULT_VERTICAL_LAUNCH_VELOCITY = 0x068,
    FIELD_VM_SET_ENTITY_DEFAULT_GRAVITY = 0x069,
    FIELD_VM_SET_ENTITY_TERMINAL_FALL_VELOCITY = 0x06A,
    FIELD_VM_SET_ENTITY_SHADOW_SUPPORT_ENABLED = 0x06B,
    FIELD_VM_BIND_ENTITY_RESOURCE = 0x06C,
    FIELD_VM_RESTORE_ENTITY_RESOURCE_STATE = 0x06D,
    FIELD_VM_WAIT_ENTITY_ANIMATION = 0x06E,
    FIELD_VM_SET_ENTITY_ANIMATION = 0x06F,
    FIELD_VM_RESTORE_ENTITY_ANIMATION = 0x070,
    FIELD_VM_SET_ENTITY_BEHAVIOR_MODE = 0x071,
    FIELD_VM_RESTORE_ENTITY_BEHAVIOR_STATE = 0x072,
    FIELD_VM_SET_ENTITY_PALETTE_PROFILE = 0x073,
    FIELD_VM_RESTORE_ENTITY_PALETTE_PROFILE = 0x074,
    FIELD_VM_START_ENTITY_PALETTE_ANIMATION = 0x075,
    FIELD_VM_STOP_ENTITY_PALETTE_ANIMATION = 0x076,
    FIELD_VM_PAUSE_ENTITY_PALETTE_ANIMATION = 0x077,
    FIELD_VM_RESUME_ENTITY_PALETTE_ANIMATION = 0x078,
    FIELD_VM_REQUEST_ENTITY_BASE_PALETTE_RELOAD = 0x079,
    FIELD_VM_WAIT_ENTITY_BASE_PALETTE_RELOAD = 0x07A,
    FIELD_VM_START_ENTITY_SCALING = 0x07B,
    FIELD_VM_START_ENTITY_TIMED_SCALING = 0x07C,
    FIELD_VM_WAIT_ENTITY_SCALING = 0x07D,
    FIELD_VM_STOP_ENTITY_SCALING = 0x07E,
    FIELD_VM_START_ENTITY_ROTATION = 0x07F,
    FIELD_VM_START_ENTITY_TIMED_ROTATION = 0x080,
    FIELD_VM_WAIT_ENTITY_ROTATION = 0x081,
    FIELD_VM_STOP_ENTITY_ROTATION = 0x082,
    FIELD_VM_START_ENTITY_MOVEMENT = 0x083,
    FIELD_VM_START_ENTITY_TIMED_MOVEMENT = 0x084,
    FIELD_VM_START_ENTITY_MOVEMENT_RELATIVE_TO_ENTITY = 0x085,
    FIELD_VM_START_ENTITY_TIMED_MOVEMENT_RELATIVE_TO_ENTITY = 0x086,
    FIELD_VM_START_ENTITY_ORBIT_AROUND_POINT = 0x087,
    FIELD_VM_START_ENTITY_TIMED_ORBIT_AROUND_POINT = 0x088,
    FIELD_VM_START_ENTITY_ORBIT_AROUND_ENTITY = 0x089,
    FIELD_VM_START_ENTITY_TIMED_ORBIT_AROUND_ENTITY = 0x08A,
    FIELD_VM_WAIT_ENTITY_MOVEMENT = 0x08B,
    FIELD_VM_CANCEL_ENTITY_MOVEMENT = 0x08C,
    FIELD_VM_START_ENTITY_VERTICAL_MOTION = 0x08D,
    FIELD_VM_START_ENTITY_VERTICAL_MOTION_TO_HEIGHT = 0x08E,
    FIELD_VM_WAIT_ENTITY_VERTICAL_MOTION = 0x08F,
    FIELD_VM_STOP_ENTITY_VERTICAL_MOTION = 0x090,
    FIELD_VM_SET_ENTITY_POSITION = 0x091,
    FIELD_VM_SET_ENTITY_ROAMING_BOUNDS = 0x092,
    FIELD_VM_ADD_ENTITY_ROAMING_PROFILE = 0x093,
    FIELD_VM_SET_ENTITY_ROAMING_BOUNDARY_CLAMP_ENABLED = 0x094,
    FIELD_VM_START_ENTITY_RANDOM_ROAMING = 0x095,
    FIELD_VM_STOP_ENTITY_RANDOM_ROAMING = 0x096,
    FIELD_VM_PAUSE_ENTITY_RANDOM_ROAMING = 0x097,
    FIELD_VM_RESUME_ENTITY_RANDOM_ROAMING = 0x098,
    FIELD_VM_CLEAR_ENTITY_ROAMING_PROFILES = 0x099,
    FIELD_VM_LOAD_ENTITY_WAYPOINT_PATH = 0x09A,
    FIELD_VM_START_ENTITY_WAYPOINT_PATH = 0x09B,
    FIELD_VM_STOP_ENTITY_WAYPOINT_PATH = 0x09C,
    FIELD_VM_PAUSE_ENTITY_WAYPOINT_PATH = 0x09D,
    FIELD_VM_RESUME_ENTITY_WAYPOINT_PATH = 0x09E,
    FIELD_VM_CLEAR_ENTITY_WAYPOINT_PATH = 0x09F,
    FIELD_VM_SET_ENTITY_FACING_DIRECTION = 0x0A0,
    FIELD_VM_FACE_ENTITY_TOWARD_ENTITY = 0x0A1,
    FIELD_VM_SPAWN_ENTITY_EFFECT_SPRITE = 0x0A2,
    FIELD_VM_REMOVE_ENTITY_EFFECT_SPRITE = 0x0A3,
    FIELD_VM_WAIT_ENTITY_EFFECT_SPRITE = 0x0A4,
    FIELD_VM_SET_FIELD_BLOCK_IDLE_BOBBING_ENABLED = 0x0A5,
    FIELD_VM_SET_FIELD_BLOCK_BOUNCE_CONTROLLER_ENABLED = 0x0A6,
    FIELD_VM_WAIT_FIELD_BLOCK_BOUNCE = 0x0A7,
    FIELD_VM_SET_ENEMY_JUMP_FIRST_STRIKE_ENABLED = 0x0A8,
    FIELD_VM_SET_ENEMY_SPIKED_JUMP_RESPONSE = 0x0A9,
    FIELD_VM_SET_ENEMY_SPECIAL_CONTACT_REMOVAL_ENABLED = 0x0AA,
    FIELD_VM_SET_ENEMY_IMMEDIATE_BATTLE_REMOVAL_ENABLED = 0x0AB,
    FIELD_VM_SET_ENTITY_SCRIPT_VALUE = 0x0AC,
    FIELD_VM_GET_ENTITY_SCRIPT_VALUE = 0x0AD,
    FIELD_VM_REJOIN_PARTY_FOLLOWER = 0x0AE,
    FIELD_VM_WAIT_PARTY_FOLLOWER_REJOINED = 0x0AF,
    FIELD_VM_DETACH_PARTY_FOLLOWER = 0x0B0,
    FIELD_VM_REUNITE_SPLIT_PARTIES = 0x0B1,
    FIELD_VM_WAIT_SPLIT_PARTY_REUNION = 0x0B2,
    FIELD_VM_START_BABY_PIGGYBACK_MOUNT = 0x0B3,
    FIELD_VM_WAIT_BABY_PIGGYBACK_MOUNT = 0x0B4,
    FIELD_VM_SET_PARTY_MEMBER_CHARACTER_ID = 0x0B5,
    FIELD_VM_RESET_PARTY_MEMBER_CHARACTERS = 0x0B6,
    FIELD_VM_SET_PARTY_INDICATOR_VISIBILITY = 0x0B7,
    FIELD_VM_SET_PARTY_MEMBER_SWITCHING_ENABLED = 0x0B8,
    FIELD_VM_LEGACY_NOOP_0B9 = 0x0B9,
    FIELD_VM_SET_FIELD_PARTY_HUD_LAYOUT = 0x0BA,
    FIELD_VM_SET_PARTY_ATTACHED_SPRITE_VISIBLE = 0x0BB,
    FIELD_VM_SET_PARTY_FIELD_MODE = 0x0BC,
    FIELD_VM_CANCEL_PARTY_ACTIONS = 0x0BD,
    FIELD_VM_WAIT_PARTY_ACTIONS_IDLE = 0x0BE,
    FIELD_VM_START_BABY_HAMMER_SWING = 0x0BF,
    FIELD_VM_WAIT_BABY_HAMMER_SWING = 0x0C0,
    FIELD_VM_FINISH_BABY_HAMMER_SWING = 0x0C1,
    FIELD_VM_PREPARE_BABY_DRILL_MARKER = 0x0C2,
    FIELD_VM_MOVE_PARTY_TO_GIMMICK_ANCHOR = 0x0C3,
    FIELD_VM_PREPARE_BROS_BALL_CANNON_LAUNCH = 0x0C4,
    FIELD_VM_LAUNCH_BROS_BALL_FROM_CANNON = 0x0C5,
    FIELD_VM_LAUNCH_BABY_PARTY_TO_ELEVATION = 0x0C6,
    FIELD_VM_DROP_BABY_PARTY_FROM_ELEVATION = 0x0C7,
    FIELD_VM_COMPLETE_AIRBORNE_BABY_TRANSFERS = 0x0C8,
    FIELD_VM_GET_PARTY_CONTROLLER_PROPERTY = 0x0C9,
    FIELD_VM_SWITCH_ACTIVE_PARTY = 0x0CA,
    FIELD_VM_WAIT_ACTIVE_PARTY_SWITCH = 0x0CB,
    FIELD_VM_SET_PARTY_PIGGYBACK_STATE = 0x0CC,
    FIELD_VM_RELEASE_ROOM_TRANSITION_CONTROL_LOCK = 0x0CD,
    FIELD_VM_SET_PARTY_LEADER_ANIMATION_OVERRIDE = 0x0CE,
    FIELD_VM_CHANGE_FIELD_ROOM_FOR_PARTY = 0x0CF,
    FIELD_VM_CHANGE_FIELD_ROOM_FOR_BOTH_PARTIES = 0x0D0,
    FIELD_VM_CHANGE_FIELD_ROOM = 0x0D1,
    FIELD_VM_TRANSFER_PARTY_BETWEEN_FIELD_SCREENS = 0x0D2,
    FIELD_VM_ENSURE_PARTY_IN_CURRENT_FIELD = 0x0D3,
    FIELD_VM_ACTIVATE_FIELD_MAP_EVENT = 0x0D4,
    FIELD_VM_START_MAP_TILE_ANIMATION = 0x0D5,
    FIELD_VM_WAIT_MAP_TILE_ANIMATION = 0x0D6,
    FIELD_VM_PAUSE_MAP_TILE_ANIMATION = 0x0D7,
    FIELD_VM_RESUME_MAP_TILE_ANIMATION = 0x0D8,
    FIELD_VM_SET_FIELD_BG_LAYERS_ENABLED = 0x0D9,
    FIELD_VM_START_CAMERA_PROFILED_MOVEMENT = 0x0DA,
    FIELD_VM_START_CAMERA_TIMED_MOVEMENT = 0x0DB,
    FIELD_VM_START_CAMERA_PROFILED_ENTITY_TRACKING = 0x0DC,
    FIELD_VM_START_CAMERA_TIMED_ENTITY_TRACKING = 0x0DD,
    FIELD_VM_WAIT_CAMERA_MOVEMENT = 0x0DE,
    FIELD_VM_STOP_CAMERA_MOVEMENT = 0x0DF,
    FIELD_VM_START_BG_LAYER_PROFILED_SCROLL = 0x0E0,
    FIELD_VM_START_BG_LAYER_TIMED_SCROLL = 0x0E1,
    FIELD_VM_WAIT_BG_LAYER_SCROLL = 0x0E2,
    FIELD_VM_STOP_BG_LAYER_SCROLL = 0x0E3,
    FIELD_VM_START_CAMERA_SHAKE = 0x0E4,
    FIELD_VM_WAIT_CAMERA_SHAKE = 0x0E5,
    FIELD_VM_STOP_CAMERA_SHAKE = 0x0E6,
    FIELD_VM_START_MAP_PALETTE_ANIMATION = 0x0E7,
    FIELD_VM_RESET_ALL_MAP_PALETTE_ANIMATIONS = 0x0E8,
    FIELD_VM_STOP_MAP_PALETTE_ANIMATION = 0x0E9,
    FIELD_VM_RESUME_MAP_PALETTE_ANIMATION = 0x0EA,
    FIELD_VM_RESTORE_MAP_BASE_PALETTES = 0x0EB,
    FIELD_VM_WAIT_MAP_PALETTE_ANIMATIONS = 0x0EC,
    FIELD_VM_START_MASTER_BRIGHTNESS_TRANSITION = 0x0ED,
    FIELD_VM_WAIT_MASTER_BRIGHTNESS_TRANSITION = 0x0EE,
    FIELD_VM_PAUSE_MASTER_BRIGHTNESS_TRANSITION = 0x0EF,
    FIELD_VM_RESUME_MASTER_BRIGHTNESS_TRANSITION = 0x0F0,
    FIELD_VM_REVERSE_MASTER_BRIGHTNESS_TRANSITION = 0x0F1,
    FIELD_VM_START_FIELD_PALETTE_ANIMATION = 0x0F2,
    FIELD_VM_WAIT_FIELD_PALETTE_ANIMATION = 0x0F3,
    FIELD_VM_PAUSE_FIELD_PALETTE_ANIMATION = 0x0F4,
    FIELD_VM_RESUME_FIELD_PALETTE_ANIMATION = 0x0F5,
    FIELD_VM_REVERSE_FIELD_PALETTE_ANIMATION = 0x0F6,
    FIELD_VM_START_PALETTE_BANK_CROSSFADE = 0x0F7,
    FIELD_VM_WAIT_PALETTE_BANK_CROSSFADE = 0x0F8,
    FIELD_VM_START_PATTERN_SCREEN_WIPE = 0x0F9,
    FIELD_VM_START_CIRCULAR_SCREEN_WIPE = 0x0FA,
    FIELD_VM_START_RECTANGULAR_SCREEN_WIPE = 0x0FB,
    FIELD_VM_WAIT_SCREEN_WIPE = 0x0FC,
    FIELD_VM_SET_FIELD_SCREEN_ALPHA_BLEND = 0x0FD,
    FIELD_VM_START_TIME_HOLE_DEPARTURE = 0x0FE,
    FIELD_VM_PREPARE_TIME_HOLE_ARRIVAL = 0x0FF,
    FIELD_VM_START_PREPARED_TIME_HOLE_ARRIVAL = 0x100,
    FIELD_VM_START_TIME_HOLE_TUNNEL = 0x101,
    FIELD_VM_START_FIELD_VERTICAL_ACCELERATING_SCROLL = 0x102,
    FIELD_VM_DECELERATE_FIELD_VERTICAL_SCROLL = 0x103,
    FIELD_VM_WAIT_FIELD_VERTICAL_SCROLL = 0x104,
    FIELD_VM_BEGIN_TIME_HOLE_TUNNEL_WHITEOUT = 0x105,
    FIELD_VM_WAIT_TIME_HOLE_TRANSITION = 0x106,
    FIELD_VM_START_ANCHORED_FIELD_EFFECT_ANIMATION = 0x107,
    FIELD_VM_WAIT_ANCHORED_FIELD_EFFECT_ANIMATION = 0x108,
    FIELD_VM_START_FIELD_EFFECT_ANIMATION = 0x109,
    FIELD_VM_WAIT_FIELD_EFFECT_ANIMATION = 0x10A,
    FIELD_VM_RETURN_TO_TITLE_SCREEN = 0x10B,
    FIELD_VM_LEGACY_NOOP_10C = 0x10C,
    FIELD_VM_WAIT_PAIRED_FIELD_READY = 0x10D,
    FIELD_VM_ENABLE_FIELD_TRIGGER_AREA = 0x10E,
    FIELD_VM_DISABLE_FIELD_TRIGGER_AREA = 0x10F,
    FIELD_VM_APPLY_SAVED_PARTY_CONFIGURATION = 0x110,
    FIELD_VM_SET_FIELD_INPUT_DISABLE_MASK = 0x111,
    FIELD_VM_SET_FIELD_EVENT_INPUT_DISABLE_MASK = 0x112,
    FIELD_VM_SET_FIELD_INPUT_CHANNEL_OVERRIDE = 0x113,
    FIELD_VM_RESET_FIELD_INPUT_CHANNEL_OVERRIDES = 0x114,
    FIELD_VM_SET_FIELD_INPUT_DIRECTION_ROTATION = 0x115,
    FIELD_VM_SET_FIELD_PARTY_CONTROL_ENABLED = 0x116,
    FIELD_VM_SET_CAMERA_FOCUS_ENTITY = 0x117,
    FIELD_VM_REMOVE_ALL_ENTITY_EFFECT_SPRITES = 0x118,
    FIELD_VM_WAIT_ALL_ENTITY_EFFECT_SPRITES = 0x119,
    FIELD_VM_LEGACY_NOOP_11A = 0x11A,
    FIELD_VM_LEGACY_NOOP_11B = 0x11B,
    FIELD_VM_START_SCRIPTED_BATTLE = 0x11C,
    FIELD_VM_START_SCRIPTED_BATTLE_WITH_TRANSITION = 0x11D,
    FIELD_VM_OPEN_PAUSE_MENU = 0x11E,
    FIELD_VM_SET_SAVE_LOCATION_ID = 0x11F,
    FIELD_VM_OPEN_SAVE_MENU = 0x120,
    FIELD_VM_OPEN_SHOP = 0x121,
    FIELD_VM_START_STAFF_CREDITS = 0x122,
    FIELD_VM_OPEN_GAME_OVER_MENU = 0x123,
    FIELD_VM_INITIALIZE_NEW_GAME_PRESET = 0x124,
    FIELD_VM_RESTORE_PARTY_MEMBER_HP = 0x125,
    FIELD_VM_ADJUST_PARTY_MEMBER_HP = 0x126,
    FIELD_VM_SET_COINS = 0x127,
    FIELD_VM_ADJUST_COINS = 0x128,
    FIELD_VM_ADJUST_BEANS = 0x129,
    FIELD_VM_ADJUST_ITEM_COUNT = 0x12A,
    FIELD_VM_GET_ROOM_COMPANION_ID = 0x12B,
    FIELD_VM_ENABLE_TOUCHSCREEN_INPUT_TRACKING = 0x12C,
    FIELD_VM_DISABLE_TOUCHSCREEN_INPUT_TRACKING = 0x12D,
    FIELD_VM_SET_RETRY_CHECKPOINT = 0x12E,
    FIELD_VM_SET_RETRY_PROGRESS = 0x12F,
    FIELD_VM_CLEAR_EXTENDED_SAVE_FLAGS = 0x130,
    FIELD_VM_SET_EXTENDED_SAVE_FLAGS = 0x131,
    FIELD_VM_PREPARE_TOUCHSCREEN_MASK_ERASE = 0x132,
    FIELD_VM_WAIT_TOUCHSCREEN_MASK_ERASE_READY = 0x133,
    FIELD_VM_START_TOUCHSCREEN_MASK_ERASE = 0x134,
    FIELD_VM_WAIT_TOUCHSCREEN_MASK_ERASE_COMPLETE = 0x135,
    FIELD_VM_PLAY_RUMBLE_PATTERN = 0x136,
    FIELD_VM_STOP_RUMBLE = 0x137,
    FIELD_VM_SET_SINGLE_PASS_COLLISION = 0x138,
    FIELD_VM_SET_TIMER_VALUE = 0x139,
    FIELD_VM_SET_TIMER_DISPLAY_POSITION = 0x13A,
    FIELD_VM_SET_TIMER_VISIBLE = 0x13B,
    FIELD_VM_SET_TIMER_COUNT_RATE = 0x13C,
    FIELD_VM_PAUSE_TIMER = 0x13D,
    FIELD_VM_RESUME_TIMER = 0x13E,
    FIELD_VM_STOP_TIMER = 0x13F,
    FIELD_VM_OPEN_SCREEN_MESSAGE = 0x140,
    FIELD_VM_OPEN_ENTITY_MESSAGE = 0x141,
    FIELD_VM_WAIT_MESSAGE_FINISHED = 0x142,
    FIELD_VM_WAIT_MESSAGE_CLOSED = 0x143,
    FIELD_VM_CLOSE_MESSAGE = 0x144,
    FIELD_VM_SET_MESSAGE_WINDOW_SLIDE_MASK_ENABLED = 0x145,
    FIELD_VM_START_MESSAGE_WINDOW_SLIDE_BY_SPEED = 0x146,
    FIELD_VM_START_MESSAGE_WINDOW_SLIDE_OVER_FRAMES = 0x147,
    FIELD_VM_WAIT_MESSAGE_WINDOW_SLIDE = 0x148,
    FIELD_VM_PLAY_FIELD_SOUND = 0x149,
    FIELD_VM_STOP_FIELD_SOUND = 0x14A,
    FIELD_VM_PLAY_BACKGROUND_MUSIC = 0x14B,
    FIELD_VM_STOP_BACKGROUND_MUSIC = 0x14C,
    FIELD_VM_LOAD_BACKGROUND_MUSIC_RESOURCE = 0x14D,
    FIELD_VM_WAIT_BACKGROUND_MUSIC_RESOURCE = 0x14E,
    FIELD_VM_ACTIVATE_BACKGROUND_MUSIC = 0x14F,
    FIELD_VM_FADE_OUT_BACKGROUND_MUSIC = 0x150,
    FIELD_VM_SET_GLOBAL_SOUND_MUTED = 0x151,
    FIELD_VM_LOAD_SOUND_GROUP_ASYNC = 0x152,
    FIELD_VM_WAIT_SOUND_GROUP_LOAD = 0x153,
    FIELD_VM_RELEASE_SOUND_GROUP = 0x154
};

enum FieldVmContextOffset {
    FIELD_VM_RESOURCE_RECORDS_OFFSET = 0x00B4,
    FIELD_VM_RESOURCE_COUNTS_OFFSET = 0x2338,
    FIELD_VM_CURRENT_ROOM_ID_OFFSET = 0x23F4,
    FIELD_VM_SPECIAL_PARTY_STATE_OFFSET = 0x23F8,
    FIELD_VM_SPECIAL_PARTY_ENTITY_ID_OFFSET = 0x23FA,
    FIELD_VM_BRIGHTNESS_FLAGS_OFFSET = 0x2402,
    FIELD_VM_BRIGHTNESS_FRAMES_OFFSET = 0x2406,
    FIELD_VM_MESSAGE_WINDOW_SLIDE_FLAGS_OFFSET = 0x249C,
    FIELD_VM_CAMERA_SHAKE_FLAGS_OFFSET = 0x2488,
    FIELD_VM_CAMERA_FOCUS_X_OFFSET = 0x2468,
    FIELD_VM_CAMERA_FOCUS_Y_OFFSET = 0x246C,
    FIELD_VM_PARTY_OWNERSHIP_MASKS_OFFSET = 0x24C0,
    FIELD_VM_INPUT_DISABLE_MASK_OFFSET = 0x24C4,
    FIELD_VM_EVENT_INPUT_DISABLE_MASK_OFFSET = 0x24C6,
    FIELD_VM_INPUT_OVERRIDES_OFFSET = 0x24C8,
    FIELD_VM_INPUT_EXCLUSION_MASKS_OFFSET = 0x24D0,
    FIELD_VM_INPUT_DIRECTION_ROTATION_OFFSET = 0x24D8,
    FIELD_VM_TRIGGER_AREAS_OFFSET = 0x24F4,
    FIELD_VM_FIELD_SYSTEM_OFFSET = 0x24FC,
    FIELD_VM_AUX_SCRIPT_STATES_OFFSET = 0x26A4,
    FIELD_VM_PAIRED_FIELD_CONTEXT_OFFSET = 0x29D4,
    FIELD_VM_ENTITY_TABLE_OFFSET = 0x29D8,
    FIELD_VM_SPECIAL_RESOURCE_RECORDS_OFFSET = 0x2A68,
    FIELD_VM_PARTY_MANAGER_OFFSET = 0x256C,
    FIELD_VM_MAP_CONTROLLER_OFFSET = 0x2500,
    FIELD_VM_ENTITY_COUNT_OFFSET = 0x2B32,
    FIELD_VM_FIELD_CONTROL_FLAGS_OFFSET = 0x2B30,
    FIELD_VM_CAMERA_TARGET_FLAGS_OFFSET = 0x2B5C,
    FIELD_VM_CAMERA_MOVEMENT_FLAGS_OFFSET = 0x2B90,
    FIELD_VM_BG_SCROLL_CONTROLLERS_OFFSET = 0x2BC4,
    FIELD_VM_SCREEN_WIPE_STATE_OFFSET = 0x2BC8,
    FIELD_VM_PALETTE_CROSSFADE_STATE_OFFSET = 0x2BCC,
    FIELD_VM_RESOURCE_RECORD_SIZE = 24
};

enum FieldVmPartyLayout {
    FIELD_VM_PARTY_CONTROLLER_OFFSET = 0x20,
    FIELD_VM_PARTY_CONTROLLER_SIZE = 0x20A4,
    FIELD_VM_PARTY_CONTROLLER_FLAGS_OFFSET = 0x50,
    FIELD_VM_PARTY_CONTROLLER_CONTROL_FLAGS_OFFSET = 0x74
};

enum FieldVmFieldSystemOffset {
    FIELD_VM_FIELD_SYSTEM_SAVE_LOCATION_OFFSET = 0x25C,
    FIELD_VM_FIELD_SYSTEM_TOUCH_STATE_OFFSET = 0x268,
    FIELD_VM_FIELD_SYSTEM_TIMER_OFFSET = 0x3A0
};

enum FieldVmPairedContextOffset {
    FIELD_VM_PAIRED_CONTEXT_TYPE_OFFSET = 0x23F0,
    FIELD_VM_PAIRED_ROOM_ID_OFFSET = 0x23F4,
    FIELD_VM_PAIRED_SCRIPT_TABLE_OFFSET = 0x24E0,
    FIELD_VM_PAIRED_SCRIPT_MANAGER_OFFSET = 0x2570,
    FIELD_VM_PAIRED_SCRIPT_FLAGS_OFFSET = 0x29B8,
    FIELD_VM_PAIRED_PARENT_ENTITY_ID_OFFSET = 0x29C1,
    FIELD_VM_PAIRED_QUEUED_SCRIPT_OFFSET = 0x29C8
};

static inline int FieldVm_GetContextType(const FieldScriptState *state) {
    return state->flag_bits.context_type;
}

static inline int FieldVm_GetParentType(const FieldScriptState *state) {
    return state->flag_bits.parent_type;
}

static inline int FieldVm_GetEffectiveParentType(const FieldScriptState *state) {
    int owner_type = state->flag_bits.owner_type;

    if (owner_type == FIELD_SCRIPT_OWNER_ENTITY) {
        return state->flag_bits.parent_type;
    }
    return owner_type;
}

static inline int FieldVm_GetOwnerSubtype(const FieldScriptState *state) {
    return state->flag_bits.owner_subtype;
}

static inline int FieldVm_GetParentEntityId(const FieldScriptState *state) {
    if (state->flag_bits.owner_type == FIELD_SCRIPT_OWNER_ENTITY) {
        return *(u8 *)((u8 *)state->owner_entity + 4);
    }
    return -1;
}

static inline int FieldVm_GetScriptState(const FieldScriptState *state) {
    if (state->flag_bits.active == 0) {
        return 0;
    }
    if (state->flag_bits.paused != 0) {
        return 3;
    }
    if (state->flag_bits.result == 3) {
        return 2;
    }
    return 1;
}

static inline int FieldVm_RetryCurrentCommand(
    ScriptVm *vm, FieldScriptState *state, u16 opcode) {
    s32 descriptor = (u16)vm->command_descriptors[opcode];
    int halfword_count =
        ((descriptor & SCRIPT_VM_HAS_RESULT) >> 5) +
        (descriptor & SCRIPT_VM_ARGUMENT_COUNT_MASK) +
        ((descriptor & SCRIPT_VM_HAS_ARGUMENT_MODES) >> 6) + 1;

    state->vm_state.script -= halfword_count;
    return SCRIPT_VM_YIELDED;
}

static inline FieldScriptState *FieldVm_GetEntityScript(FieldEntity *entity) {
    return (FieldScriptState *)((u8 *)entity + 0x20);
}

static inline FieldScriptState *FieldVm_GetAuxScript(FieldVmRuntime *runtime,
                                                      int slot) {
    return (FieldScriptState *)(
        runtime->field_context + FIELD_VM_AUX_SCRIPT_STATES_OFFSET) + slot;
}

static inline FieldEntity *FieldVm_GetEntityByIndex(u8 *field_context,
                                                    int entity_index) {
    FieldEntity **entities = (FieldEntity **)(
        field_context + FIELD_VM_ENTITY_TABLE_OFFSET);

    return entities[entity_index];
}

static inline void FieldVm_VisitMatchingEntityScripts(
    FieldVmRuntime *runtime, FieldScriptState *caller, int selector,
    int operation, int *result) {
    u8 *field_context = runtime->field_context;
    int parent_type = selector;
    int entity_count = *(u8 *)(
        field_context + FIELD_VM_ENTITY_COUNT_OFFSET);
    int parent_entity_id;
    int index;

    if (selector == -1) {
        parent_type = caller->flag_bits.owner_type;
    }
    if (parent_type == FIELD_SCRIPT_OWNER_ENTITY) {
        parent_entity_id = *(u8 *)((u8 *)caller->owner_entity + 4);
    } else {
        parent_entity_id = -1;
    }

    switch (operation) {
    case FIELD_VM_WAIT_MATCHING_ENTITY_SCRIPTS:
        for (index = 0; index < entity_count; index++) {
            FieldEntity *entity = ((FieldEntity **)(
                field_context + FIELD_VM_ENTITY_TABLE_OFFSET))[index];
            FieldScriptState *target =
                (FieldScriptState *)((u8 *)entity + 0x20);

            if (target->flag_bits.active != 0 &&
                (parent_type == target->flag_bits.parent_type ||
                 (parent_type == FIELD_SCRIPT_OWNER_ENTITY &&
                  target->parent_entity_id == parent_entity_id))) {
                s32 descriptor = (u16)
                    runtime->vm.command_descriptors[operation];
                int halfword_count =
                    ((descriptor & SCRIPT_VM_HAS_RESULT) >> 5) +
                    (descriptor & SCRIPT_VM_ARGUMENT_COUNT_MASK) +
                    ((descriptor & SCRIPT_VM_HAS_ARGUMENT_MODES) >> 6) + 1;

                caller->vm_state.script -= halfword_count;
                *result = SCRIPT_VM_YIELDED;
                return;
            }
        }
        break;

    case FIELD_VM_STOP_MATCHING_ENTITY_SCRIPTS:
        for (index = 0; index < entity_count; index++) {
            FieldEntity *entity = ((FieldEntity **)(
                field_context + FIELD_VM_ENTITY_TABLE_OFFSET))[index];
            FieldScriptState *target =
                (FieldScriptState *)((u8 *)entity + 0x20);

            if (target->flag_bits.active != 0 &&
                (parent_type == target->flag_bits.parent_type ||
                 (parent_type == FIELD_SCRIPT_OWNER_ENTITY &&
                  target->parent_entity_id == parent_entity_id))) {
                entity->stop_script();
            }
        }
        break;

    case FIELD_VM_PAUSE_MATCHING_ENTITY_SCRIPTS:
        for (index = 0; index < entity_count; index++) {
            FieldEntity *entity = ((FieldEntity **)(
                field_context + FIELD_VM_ENTITY_TABLE_OFFSET))[index];
            FieldScriptState *target =
                (FieldScriptState *)((u8 *)entity + 0x20);

            if (target->flag_bits.active != 0 &&
                (parent_type == target->flag_bits.parent_type ||
                 (parent_type == FIELD_SCRIPT_OWNER_ENTITY &&
                  target->parent_entity_id == parent_entity_id))) {
                entity->pause_script();
            }
        }
        break;

    case FIELD_VM_RESUME_MATCHING_ENTITY_SCRIPTS:
        for (index = 0; index < entity_count; index++) {
            FieldEntity *entity = ((FieldEntity **)(
                field_context + FIELD_VM_ENTITY_TABLE_OFFSET))[index];
            FieldScriptState *target =
                (FieldScriptState *)((u8 *)entity + 0x20);

            if (target->flag_bits.active != 0 &&
                (parent_type == target->flag_bits.parent_type ||
                 (parent_type == FIELD_SCRIPT_OWNER_ENTITY &&
                  target->parent_entity_id == parent_entity_id))) {
                entity->resume_script();
            }
        }
        break;
    }
}

static inline void FieldVm_StartEntityScript(FieldScriptState *caller,
                                             FieldEntity *entity,
                                             FieldScriptState *target,
                                             const u16 *script,
                                             int chain_if_active) {
    if (!chain_if_active || target->flag_bits.active == 0) {
        entity->stop_script();
        FieldScript_Begin(
            target, caller,
            caller->flag_bits.context_type,
            script);
    } else {
        int parent_type;
        int parent_entity_id;

        target->queued_script = script;
        parent_type = caller->flag_bits.owner_type;
        if (parent_type == FIELD_SCRIPT_OWNER_ENTITY) {
            parent_type = caller->flag_bits.parent_type;
        }
        target->flag_bits.queued_parent_type = parent_type;
        if (caller->flag_bits.owner_type == FIELD_SCRIPT_OWNER_ENTITY) {
            parent_entity_id = *(u8 *)((u8 *)caller->owner_entity + 4);
        } else {
            parent_entity_id = -1;
        }
        target->queued_parent_entity_id = parent_entity_id;
    }
}

static inline void FieldVm_StartCurrentEntityScript(
    FieldScriptState *caller, FieldEntity *entity, FieldScriptState *target,
    int chain_if_active) {
    if (!chain_if_active || target->flag_bits.active == 0) {
        entity->stop_script();
        FieldScript_Begin(
            target, caller, caller->flag_bits.context_type,
            caller->vm_state.script);
    } else {
        int parent_type;
        int parent_entity_id;

        target->queued_script = caller->vm_state.script;
        parent_type = caller->flag_bits.owner_type;
        if (parent_type == FIELD_SCRIPT_OWNER_ENTITY) {
            parent_type = caller->flag_bits.parent_type;
        }
        target->flag_bits.queued_parent_type = parent_type;
        if (caller->flag_bits.owner_type == FIELD_SCRIPT_OWNER_ENTITY) {
            parent_entity_id = *(u8 *)((u8 *)caller->owner_entity + 4);
        } else {
            parent_entity_id = -1;
        }
        target->queued_parent_entity_id = parent_entity_id;
    }
}

static inline void FieldVm_StartPairedScript(u8 *field_context,
                                             FieldScriptState *caller,
                                             s32 *arguments) {
    u8 *paired = *(u8 **)(field_context +
                         FIELD_VM_PAIRED_FIELD_CONTEXT_OFFSET);

    if (paired != 0 &&
        *(u16 *)(paired + FIELD_VM_PAIRED_CONTEXT_TYPE_OFFSET) == 0x47 &&
        *(u16 *)(paired + FIELD_VM_PAIRED_ROOM_ID_OFFSET) ==
            arguments[0]) {
        if (arguments[2] == 0 ||
            ((FieldScriptFlagBits *)(
                 paired + FIELD_VM_PAIRED_SCRIPT_FLAGS_OFFSET))->active == 0) {
            FieldScriptManager_StartAuxiliary(
                (FieldScriptManager *)(paired + FIELD_VM_PAIRED_SCRIPT_MANAGER_OFFSET), caller,
                (s16)arguments[1]);
        } else {
            const u16 *script_table;
            const u16 *queued_script;
            int parent_type;
            int parent_entity_id;

            if (arguments[1] == -1) {
                queued_script = 0;
            } else {
                script_table = *(const u16 **)(
                    paired + FIELD_VM_PAIRED_SCRIPT_TABLE_OFFSET);
                queued_script = (const u16 *)(
                    (const u8 *)script_table +
                    (script_table + (arguments[1] & 0xFFFF))[9]);
            }
            *(const u16 **)(paired + FIELD_VM_PAIRED_QUEUED_SCRIPT_OFFSET) =
                queued_script;
            parent_type = caller->flag_bits.owner_type;
            if (parent_type == FIELD_SCRIPT_OWNER_ENTITY) {
                parent_type = caller->flag_bits.parent_type;
            }
            ((FieldScriptFlagBits *)(
                 *(u8 **)(field_context +
                          FIELD_VM_PAIRED_FIELD_CONTEXT_OFFSET) +
                 FIELD_VM_PAIRED_SCRIPT_FLAGS_OFFSET))
                ->queued_parent_type = parent_type;
            if (caller->flag_bits.owner_type == FIELD_SCRIPT_OWNER_ENTITY) {
                parent_entity_id = *(u8 *)((u8 *)caller->owner_entity + 4);
            } else {
                parent_entity_id = -1;
            }
            *(s8 *)(*(u8 **)(field_context +
                             FIELD_VM_PAIRED_FIELD_CONTEXT_OFFSET) +
                    FIELD_VM_PAIRED_PARENT_ENTITY_ID_OFFSET) =
                parent_entity_id;
        }
    }
}

static inline FieldRuntimeEntity *FieldVm_GetRuntimeEntity(
    FieldEntity *entity) {
    return (FieldRuntimeEntity *)entity;
}

static inline FieldSpecialPartyState *FieldVm_GetSpecialPartyState(
    u8 *field_context) {
    return (FieldSpecialPartyState *)(
        field_context + FIELD_VM_SPECIAL_PARTY_STATE_OFFSET);
}

static inline FieldBrightnessTransitionFlags *
FieldVm_GetBrightnessTransitionFlags(u8 *field_context) {
    return (FieldBrightnessTransitionFlags *)(
        field_context + FIELD_VM_BRIGHTNESS_FLAGS_OFFSET);
}

static inline u8 *FieldVm_GetPartyManager(FieldVmRuntime *runtime) {
    return *(u8 **)(runtime->field_context + FIELD_VM_PARTY_MANAGER_OFFSET);
}

static inline FieldMapController *FieldVm_GetMapController(
    FieldVmRuntime *runtime) {
    return *(FieldMapController **)(
        runtime->field_context + FIELD_VM_MAP_CONTROLLER_OFFSET);
}

static inline u8 *FieldVm_GetPartyController(u8 *party_manager,
                                             int party_side) {
    return party_manager + FIELD_VM_PARTY_CONTROLLER_OFFSET +
        FIELD_VM_PARTY_CONTROLLER_SIZE * party_side;
}

#define FieldVm_GetPartyControllerFlags(party_controller)                  \
    ((FieldPartyControllerFlags *)((party_controller) +                    \
        FIELD_VM_PARTY_CONTROLLER_FLAGS_OFFSET))

#define FieldVm_GetPresentPartyMask(field_context) \
    (((FieldSpecialPartyState *)((field_context) + \
        FIELD_VM_SPECIAL_PARTY_STATE_OFFSET))->present_party_mask)

#define FieldVm_GetPartyManagerFlags(party_manager)                        \
    ((FieldPartyManagerFlags *)((party_manager) + 4))

#define FieldVm_GetActivePartySide(party_manager)                          \
    (FieldVm_GetPartyManagerFlags(party_manager)->active_party_side)

static inline void FieldVm_ResolveMenuPartyContext(FieldVmRuntime *runtime,
                                                   int *requested_context) {
    FieldMenuPartyManager *party_manager;
    u8 *leader_entity;
    u32 interaction_flags;
    int active_party_side;
    int resolved_context = 0;

    if (*requested_context != -1) {
        return;
    }

    party_manager = *(FieldMenuPartyManager **)(runtime->field_context +
                             FIELD_VM_PARTY_MANAGER_OFFSET);
    active_party_side = party_manager->flags.active_party_side;
    leader_entity = party_manager->controllers[active_party_side].leader_entity;
    interaction_flags = *(u32 *)(leader_entity + 0x520);
    if (((interaction_flags << 10) >> 28) == 6) {
        resolved_context = 2;
    } else if (party_manager->controllers[active_party_side].flags.
                   special_contact_mode != 0) {
        resolved_context = 1;
    }
    *requested_context = resolved_context;
}

static inline int FieldVm_GetPartyControllerProperty(u8 *party_controller,
                                                     int property_id) {
    switch (property_id) {
    case 0:
        return FieldVm_GetPartyControllerFlags(party_controller)->
            movement_active;
    case 1: {
        int member = FieldVm_GetPartyControllerFlags(party_controller)->
            active_member;
        u8 *entity = *(u8 **)(party_controller + 0x48 +
                              member * sizeof(void *));

        return *(u8 *)(entity + 0x180);
    }
    case 2:
        return FieldVm_GetPartyControllerFlags(party_controller)->
            movement_state;
    case 3: {
        u8 *entity = *(u8 **)(party_controller + 0x48);
        u16 animation_id = *(u16 *)(entity + 0x17C);

        if (animation_id == 19) {
            return 0;
        }
        if (animation_id == 20) {
            return 1;
        }
        if (animation_id == 21) {
            FieldEntityAnimationRuntimeFlags *animation_flags =
                *(FieldEntityAnimationRuntimeFlags **)(entity + 0x59C);

            return animation_flags->alternate_variant ? 3 : 2;
        }
        return -1;
    }
    case 4: {
        FieldPartyControllerAuxFlags *aux_flags =
            (FieldPartyControllerAuxFlags *)(party_controller + 0x98);

        return aux_flags->secondary_action_active;
    }
    default:
        return 0;
    }
}

static inline bool FieldVm_IsTwoDimensionalEntity(
    const FieldRuntimeEntity *entity) {
    return entity->base.property_00a_bits.subtype == 8;
}

static inline FieldInteractionFlags *FieldVm_GetInteractionFlags(
    FieldRuntimeEntity *entity) {
    return (FieldInteractionFlags *)((u8 *)entity + 0x520);
}

static inline void FieldVm_SetContactDirectionFilter(
    FieldRuntimeEntity *entity, int script_mask) {
    entity->contact_direction_flags.enabled_mask = 0;
    if ((script_mask & 0x01) != 0) {
        entity->contact_direction_flags.enabled_mask |= 0x04;
    }
    if ((script_mask & 0x04) != 0) {
        entity->contact_direction_flags.enabled_mask |= 0x01;
    }
    if ((script_mask & 0x08) != 0) {
        entity->contact_direction_flags.enabled_mask |= 0x02;
    }
    if ((script_mask & 0x02) != 0) {
        entity->contact_direction_flags.enabled_mask |= 0x08;
    }
    if ((script_mask & 0x10) != 0) {
        entity->contact_direction_flags.enabled_mask |= 0x20;
    }
    if ((script_mask & 0x20) != 0) {
        entity->contact_direction_flags.enabled_mask |= 0x10;
    }
}

static inline void FieldVm_SetNavigationBounds(FieldRuntimeEntity *entity,
                                               const s32 *arguments) {
    s16 vertical_extent = (s16)arguments[5];
    s16 height = (s16)arguments[4];
    s16 width = (s16)arguments[3];
    s16 maximum_y = (s16)arguments[2];

    entity->navigation_min_x = (s16)arguments[1] << FX32B_INT;
    entity->navigation_max_x = entity->navigation_min_x +
        (width << FX32B_INT);
    entity->navigation_max_y = maximum_y << FX32B_INT;
    entity->navigation_min_y = entity->navigation_max_y -
        (height << FX32B_INT);
    entity->navigation_vertical_extent = vertical_extent << FX32B_INT;
}

static inline void FieldVm_SetBodyBounds(FieldRuntimeEntity *entity,
                                         const s32 *arguments) {
    s16 vertical_extent = (s16)arguments[5];
    s16 height = (s16)arguments[4];
    s16 width = (s16)arguments[3];
    s16 maximum_y = (s16)arguments[2];

    entity->body_min_x = (s16)arguments[1] << FX32B_INT;
    entity->body_max_x = entity->body_min_x +
        (width << FX32B_INT);
    entity->body_max_y = maximum_y << FX32B_INT;
    entity->body_min_y = entity->body_max_y -
        (height << FX32B_INT);
    entity->body_vertical_extent = vertical_extent << FX32B_INT;
    entity->body_center_y =
        (entity->body_min_y + entity->body_max_y) / 2;
    entity->body_corner_angles[0] =
        FX_Atan2(entity->body_min_x, entity->body_center_y);
    entity->body_corner_angles[1] =
        FX_Atan2(entity->body_min_x, -entity->body_center_y);
    entity->body_corner_angles[2] =
        FX_Atan2(entity->body_max_x, -entity->body_center_y);
    entity->body_corner_angles[3] =
        FX_Atan2(entity->body_max_x, entity->body_center_y);
}

static inline void FieldVm_SetCollisionPolicy(FieldRuntimeEntity *entity,
                                              const s32 *arguments) {
    int can_displace_other = arguments[4] != 0;
    int can_be_displaced = arguments[3] != 0;
    int solid = arguments[2] != 0;
    int shift = 3 * arguments[1];

    entity->collision_policy &= ~(7 << shift);
    entity->collision_policy |= ((can_be_displaced << 1) | solid |
        (can_displace_other << 2)) << shift;
}

static inline void FieldVm_SetRenderPriorities(FieldRuntimeEntity *entity,
                                               const s32 *arguments) {
    int subtype = entity->base.property_00a_bits.subtype;

    if (subtype == 8) {
        s8 priority_3 = (s8)arguments[4];
        s8 priority_2 = (s8)arguments[3];
        s8 priority_1 = (s8)arguments[2];
        s8 priority_0 = (s8)arguments[1];

        if (entity->render_object != 0) {
            entity->render_object->overlap_priorities[0] = priority_0;
            entity->render_object->overlap_priorities[1] = priority_1;
            entity->render_object->overlap_priorities[2] = priority_2;
            entity->render_object->overlap_priorities[3] = priority_3;
        }
        return;
    }

    {
        s8 priority_0;
        s8 priority_1;
        s8 priority_2;
        s8 priority_3;
        s8 auxiliary_priority = (s8)arguments[5];
        priority_3 = (s8)arguments[4];
        priority_2 = (s8)arguments[3];
        priority_1 = (s8)arguments[2];
        priority_0 = (s8)arguments[1];

        if (entity->auxiliary_render_object != 0) {
            if (auxiliary_priority == -1) {
                entity->runtime_flags |=
                    FIELD_ENTITY_AUTO_AUXILIARY_PRIORITY;
            } else {
                entity->runtime_flags &=
                    ~FIELD_ENTITY_AUTO_AUXILIARY_PRIORITY;
                entity->auxiliary_render_object->overlap_priorities[0] =
                    auxiliary_priority;
                entity->field_state_flags =
                    (entity->field_state_flags & ~(3 << 26)) |
                    ((auxiliary_priority & 3) << 26);
            }
        }

        if (entity->render_object == 0) {
            return;
        }

        if (priority_0 == -1) {
            entity->runtime_flags |= FIELD_ENTITY_AUTO_PRIORITY_0;
        } else {
            entity->runtime_flags &= ~FIELD_ENTITY_AUTO_PRIORITY_0;
            entity->render_object->overlap_priorities[0] = priority_0;
        }
        if (priority_1 == -1) {
            entity->runtime_flags |= FIELD_ENTITY_AUTO_PRIORITY_1;
        } else {
            entity->runtime_flags &= ~FIELD_ENTITY_AUTO_PRIORITY_1;
            entity->render_object->overlap_priorities[1] = priority_1;
        }
        if (priority_2 == -1) {
            entity->runtime_flags |= FIELD_ENTITY_AUTO_PRIORITY_2;
        } else {
            entity->runtime_flags &= ~FIELD_ENTITY_AUTO_PRIORITY_2;
            entity->render_object->overlap_priorities[2] = priority_2;
        }
        if (priority_3 == -1) {
            entity->runtime_flags |= FIELD_ENTITY_AUTO_PRIORITY_3;
        } else {
            entity->runtime_flags &= ~FIELD_ENTITY_AUTO_PRIORITY_3;
            entity->render_object->overlap_priorities[3] = priority_3;
        }
    }
}

static inline s32 FieldVm_DecodeWideArgument(
    const ScriptVmCommand *command, int argument_index) {
    s32 value = command->arguments[argument_index];

    if ((command->argument_modes & (1 << argument_index)) == 0) {
        value = (value & 0xFFFF) +
            (command->arguments[argument_index + 1] << 16);
    }
    return value;
}

static inline int FieldVm_DegreesToTurn16(int degrees) {
    return (degrees << 16) / 360;
}

static inline const void *FieldVm_GetResourceRecord(
    u8 *field_context, FieldRuntimeEntity *entity, int resource_index) {
    int resource_set = entity->base.property_00a_bits.resource_set;
    int count = *(int *)(field_context + FIELD_VM_RESOURCE_COUNTS_OFFSET +
                         resource_set * sizeof(int));

    if (resource_index < count) {
        return *(const u8 **)(field_context + FIELD_VM_RESOURCE_RECORDS_OFFSET +
                              resource_set * sizeof(void *)) +
            resource_index * FIELD_VM_RESOURCE_RECORD_SIZE;
    }
    return 0;
}

static inline s32 *FieldVm_GetCommandArguments(ScriptVmCommand *command) {
    return command->arguments;
}

int FieldVm_DispatchCommand(ScriptVm *vm, ScriptVmState *base_state,
                            ScriptVmCommand *command) {
    FieldVmRuntime *runtime = (FieldVmRuntime *)vm;
    FieldScriptState *state = (FieldScriptState *)base_state;
    u8 *field_context = runtime->field_context;
    u8 *party_manager = *(u8 **)(field_context +
                                 FIELD_VM_PARTY_MANAGER_OFFSET);
    FieldSystem *field_system = *(FieldSystem **)(field_context +
                                    FIELD_VM_FIELD_SYSTEM_OFFSET);
    s32 *arguments = FieldVm_GetCommandArguments(command);
    FieldMapController *map_controller;
    FieldScriptState *target;
    FieldEntity *entity;
    FieldRuntimeEntity *runtime_entity;
    u8 *party_controller;
    const u16 *script;
    int owner_type;
    FieldCircularWipeParameters circular_wipe_parameters;
    FieldRectangularWipeParameters rectangular_wipe_parameters;
    int result = SCRIPT_VM_CONTINUE;

    switch (command->opcode) {
    case FIELD_VM_BRANCH_IF_OWNER_SUBTYPE:
        owner_type = FieldVm_GetContextType(state);
        if (owner_type == FIELD_SCRIPT_OWNER_PRIMARY ||
            owner_type == FIELD_SCRIPT_OWNER_SECONDARY) {
            owner_type = FieldVm_GetOwnerSubtype(state);
            if (owner_type == arguments[0]) {
                state->vm_state.script += arguments[1];
            }
        }
        break;

    case FIELD_VM_BRANCH_RELATIVE:
        state->vm_state.script += arguments[0];
        break;

    case FIELD_VM_SET_AUX_SCRIPT_ENABLED:
    case FIELD_VM_START_AUX_SCRIPT:
    case FIELD_VM_WAIT_AUX_SCRIPT:
    case FIELD_VM_STOP_AUX_SCRIPT:
    case FIELD_VM_PAUSE_AUX_SCRIPT:
    case FIELD_VM_RESUME_AUX_SCRIPT:
    case FIELD_VM_GET_AUX_SCRIPT_STATE:
        target = FieldVm_GetAuxScript(runtime, arguments[0]);
        switch (command->opcode) {
        case FIELD_VM_SET_AUX_SCRIPT_ENABLED:
            target->flag_bits.enabled = arguments[1];
            break;

        case FIELD_VM_START_AUX_SCRIPT:
            if (target->flag_bits.enabled != 0) {
                script = state->vm_state.script + arguments[1];
                if (arguments[2] == 0 ||
                    target->flag_bits.active == 0) {
                    FieldScript_Begin(target, state, 0, script);
                } else {
                    target->queued_script = script;
                    target->flag_bits.queued_parent_type =
                        FieldVm_GetEffectiveParentType(state);
                    target->queued_parent_entity_id =
                        FieldVm_GetParentEntityId(state);
                }
            }
            break;

        case FIELD_VM_WAIT_AUX_SCRIPT:
            if (target->flag_bits.active != 0) {
                result = FieldVm_RetryCurrentCommand(
                    vm, state, command->opcode);
                break;
            }
            break;

        case FIELD_VM_STOP_AUX_SCRIPT:
            target->flag_bits.active = 0;
            target->queued_script = 0;
            break;

        case FIELD_VM_PAUSE_AUX_SCRIPT:
            target->flag_bits.paused = 1;
            break;

        case FIELD_VM_RESUME_AUX_SCRIPT:
            target->flag_bits.paused = 0;
            break;

        case FIELD_VM_GET_AUX_SCRIPT_STATE:
            VM_WriteVariable(
                command->result_variable,
                FieldVm_GetScriptState(target),
                vm, base_state);
            break;
        }
        break;

    case FIELD_VM_START_PAIRED_FIELD_SCRIPT:
        FieldVm_StartPairedScript(field_context, state, arguments);
        break;

    case FIELD_VM_START_INLINE_ENTITY_SCRIPT:
    case FIELD_VM_START_INLINE_ENTITY_SCRIPT_AND_WAIT:
    case FIELD_VM_START_RELATIVE_ENTITY_SCRIPT:
    case FIELD_VM_WAIT_ENTITY_SCRIPT:
    case FIELD_VM_STOP_ENTITY_SCRIPT:
    case FIELD_VM_PAUSE_ENTITY_SCRIPT:
    case FIELD_VM_RESUME_ENTITY_SCRIPT:
    case FIELD_VM_GET_ENTITY_SCRIPT_STATE:
    case FIELD_VM_GET_ENTITY_PROPERTY:
    case FIELD_VM_SET_ENTITY_ENABLED:
    case FIELD_VM_SET_ENTITY_SCRIPT_VALUE:
    case FIELD_VM_GET_ENTITY_SCRIPT_VALUE:
        entity = FieldScript_ResolveEntity(
            runtime, state, arguments[0]);
        target = FieldVm_GetEntityScript(entity);
        switch (command->opcode) {
        case FIELD_VM_START_INLINE_ENTITY_SCRIPT:
        case FIELD_VM_START_INLINE_ENTITY_SCRIPT_AND_WAIT:
            if (state->flag_bits.inline_wait == 0) {
                FieldVm_StartCurrentEntityScript(
                    state, entity, target, arguments[1]);
                if (command->opcode ==
                    FIELD_VM_START_INLINE_ENTITY_SCRIPT_AND_WAIT) {
                    state->flag_bits.inline_wait = 1;
                }
            }
            if (state->flag_bits.inline_wait != 0 &&
                target->flag_bits.active != 0) {
                result = FieldVm_RetryCurrentCommand(
                    vm, state,
                    command->opcode);
                break;
            }
            state->vm_state.script += arguments[2];
            state->flag_bits.inline_wait = 0;
            break;

        case FIELD_VM_START_RELATIVE_ENTITY_SCRIPT:
            script = state->vm_state.script + arguments[1];
            FieldVm_StartEntityScript(
                state, entity, target, script, arguments[2]);
            break;

        case FIELD_VM_WAIT_ENTITY_SCRIPT:
            if (target->flag_bits.active != 0) {
                result = FieldVm_RetryCurrentCommand(
                    vm, state, command->opcode);
                break;
            }
            break;

        case FIELD_VM_STOP_ENTITY_SCRIPT:
            entity->stop_script();
            break;

        case FIELD_VM_PAUSE_ENTITY_SCRIPT:
            entity->pause_script();
            break;

        case FIELD_VM_RESUME_ENTITY_SCRIPT:
            entity->resume_script();
            break;

        case FIELD_VM_GET_ENTITY_SCRIPT_STATE:
            VM_WriteVariable(
                command->result_variable,
                FieldVm_GetScriptState(target),
                vm, base_state);
            break;

        case FIELD_VM_GET_ENTITY_PROPERTY:
            VM_WriteVariable(
                command->result_variable,
                entity->get_property(arguments[1]),
                vm, base_state);
            break;

        case FIELD_VM_SET_ENTITY_ENABLED: {
            entity->property_00a_bits.property_00a_flag_00 =
                arguments[1] != 0;
            break;
        }

        case FIELD_VM_SET_ENTITY_SCRIPT_VALUE:
            target->script_values[arguments[1]] =
                (s16)arguments[2];
            break;

        case FIELD_VM_GET_ENTITY_SCRIPT_VALUE:
            VM_WriteVariable(
                command->result_variable,
                target->script_values[arguments[1]],
                vm, base_state);
            break;
        }
        break;

    case FIELD_VM_SET_ENTITY_VISIBLE:
    case FIELD_VM_SET_ENTITY_OFFSCREEN_CONTACT_RETENTION_ENABLED:
    case FIELD_VM_SET_ENTITY_RESERVED_STATE_FLAG:
    case FIELD_VM_SET_ENTITY_SEMITRANSPARENT:
    case FIELD_VM_SET_ENTITY_RENDER_LAYER:
    case FIELD_VM_LEGACY_NOOP_061:
    case FIELD_VM_SET_ENTITY_RENDER_ORDER_PRIORITIES:
    case FIELD_VM_SET_ENTITY_INTERACTION_BOUNDS:
    case FIELD_VM_RESTORE_ENTITY_INTERACTION_BOUNDS:
    case FIELD_VM_SET_ENTITY_ANIMATION_SPEED:
    case FIELD_VM_SET_ENTITY_LOCOMOTION_PARAMETERS:
    case FIELD_VM_BIND_ENTITY_RESOURCE:
    case FIELD_VM_RESTORE_ENTITY_RESOURCE_STATE:
    case FIELD_VM_WAIT_ENTITY_ANIMATION:
    case FIELD_VM_SET_ENTITY_ANIMATION:
    case FIELD_VM_RESTORE_ENTITY_ANIMATION:
    case FIELD_VM_SET_ENTITY_BEHAVIOR_MODE:
    case FIELD_VM_RESTORE_ENTITY_BEHAVIOR_STATE:
    case FIELD_VM_SET_ENTITY_PALETTE_PROFILE:
    case FIELD_VM_RESTORE_ENTITY_PALETTE_PROFILE:
    case FIELD_VM_START_ENTITY_PALETTE_ANIMATION:
    case FIELD_VM_STOP_ENTITY_PALETTE_ANIMATION:
    case FIELD_VM_PAUSE_ENTITY_PALETTE_ANIMATION:
    case FIELD_VM_RESUME_ENTITY_PALETTE_ANIMATION:
    case FIELD_VM_REQUEST_ENTITY_BASE_PALETTE_RELOAD:
    case FIELD_VM_WAIT_ENTITY_BASE_PALETTE_RELOAD:
    case FIELD_VM_START_ENTITY_SCALING:
    case FIELD_VM_START_ENTITY_TIMED_SCALING:
    case FIELD_VM_WAIT_ENTITY_SCALING:
    case FIELD_VM_STOP_ENTITY_SCALING:
    case FIELD_VM_START_ENTITY_ROTATION:
    case FIELD_VM_START_ENTITY_TIMED_ROTATION:
    case FIELD_VM_WAIT_ENTITY_ROTATION:
    case FIELD_VM_STOP_ENTITY_ROTATION:
    case FIELD_VM_START_ENTITY_MOVEMENT:
    case FIELD_VM_START_ENTITY_TIMED_MOVEMENT:
    case FIELD_VM_START_ENTITY_MOVEMENT_RELATIVE_TO_ENTITY:
    case FIELD_VM_START_ENTITY_TIMED_MOVEMENT_RELATIVE_TO_ENTITY:
    case FIELD_VM_START_ENTITY_ORBIT_AROUND_POINT:
    case FIELD_VM_START_ENTITY_TIMED_ORBIT_AROUND_POINT:
    case FIELD_VM_START_ENTITY_ORBIT_AROUND_ENTITY:
    case FIELD_VM_START_ENTITY_TIMED_ORBIT_AROUND_ENTITY:
    case FIELD_VM_WAIT_ENTITY_MOVEMENT:
    case FIELD_VM_CANCEL_ENTITY_MOVEMENT:
    case FIELD_VM_SET_ENTITY_POSITION:
    case FIELD_VM_SET_ENTITY_FACING_DIRECTION:
    case FIELD_VM_SPAWN_ENTITY_EFFECT_SPRITE:
    case FIELD_VM_REMOVE_ENTITY_EFFECT_SPRITE:
    case FIELD_VM_WAIT_ENTITY_EFFECT_SPRITE:
        entity = FieldScript_ResolveEntity(
            runtime, state, arguments[0]);
        runtime_entity = FieldVm_GetRuntimeEntity(entity);
        switch (command->opcode) {
        case FIELD_VM_SET_ENTITY_VISIBLE: {
            int visible = arguments[1] != 0;

            entity->set_visible(visible);
            break;
        }

        case FIELD_VM_SET_ENTITY_OFFSCREEN_CONTACT_RETENTION_ENABLED:
            runtime_entity->base_state_flag_bits.retain_offscreen_contact =
                arguments[1] != 0;
            break;

        case FIELD_VM_SET_ENTITY_RESERVED_STATE_FLAG:
            runtime_entity->base_state_flag_bits.reserved_state =
                arguments[1] != 0;
            break;
        case FIELD_VM_SET_ENTITY_SEMITRANSPARENT:
            runtime_entity->render_object->state_flag_bits.semitransparent =
                arguments[1];
            break;

        case FIELD_VM_SET_ENTITY_RENDER_LAYER: {
            u8 render_layer = (u8)arguments[1];

            if (runtime_entity->render_object != 0) {
                runtime_entity->render_object->sort_key_bits.layer =
                    render_layer;
            }
            break;
        }

        case FIELD_VM_SET_ENTITY_RENDER_ORDER_PRIORITIES:
            FieldVm_SetRenderPriorities(runtime_entity, arguments);
            break;

        case FIELD_VM_SET_ENTITY_INTERACTION_BOUNDS:
            FieldEntity_SetInteractionBounds(
                entity, (s16)arguments[1],
                (s16)arguments[2], (u16)arguments[3],
                (u16)arguments[4], (u16)arguments[5]);
            break;

        /*
         * The outer table never sends opcode 0x064 here, but the original
         * compiler retained this unreachable legacy handler in the shared
         * entity-family switch.
         */
        case FIELD_VM_LEGACY_NOOP_064:
            runtime_entity->interaction_vertical_extent =
                arguments[1] << FX32B_INT;
            break;

        case FIELD_VM_RESTORE_ENTITY_INTERACTION_BOUNDS:
            FieldEntity_RefreshInteractionBounds((FieldRuntimeEntity *)entity);
            break;

        case FIELD_VM_SET_ENTITY_ANIMATION_SPEED: {
            s16 animation_speed = (s16)arguments[1];

            runtime_entity->animation_speed = animation_speed;
            func_0200940c(
                runtime_entity->render_object, animation_speed);
            break;
        }

        case FIELD_VM_BIND_ENTITY_RESOURCE:
            if (arguments[2] != -1 && arguments[4] != 0) {
                runtime_entity->saved_animation_id =
                    runtime_entity->animation_id;
                runtime_entity->saved_presentation_flag_bits.has_saved_resource_animation =
                    1;
            }
            if (arguments[1] != -1) {
                const void *resource_record = FieldVm_GetResourceRecord(
                    field_context, runtime_entity, arguments[1]);

                if (arguments[4] != 0) {
                    runtime_entity->saved_resource_index =
                        runtime_entity->resource_index;
                    runtime_entity->saved_presentation_flag_bits.has_saved_resource =
                        1;
                }
                runtime_entity->resource_index = (u8)arguments[1];
                FieldEntity_RebindRendererResources(
                    runtime_entity, (const FieldPrimaryResource *)resource_record, 0, 0,
                    arguments[2] == -1 ? 0 : arguments[2], 1, 0);
            } else if (arguments[2] != -1) {
                FieldEntity_SetResourceAnimation(
                    runtime_entity, arguments[2], -1, 1, 1);
            }
            if (arguments[3] != -1) {
                FieldTimedRenderer_SetLoopCount(
                    (FieldAnimationRenderer *)runtime_entity->render_object, arguments[3]);
            }
            break;

        case FIELD_VM_RESTORE_ENTITY_RESOURCE_STATE:
            if (runtime_entity->saved_presentation_flag_bits.has_saved_resource !=
                0) {
                const void *resource_record;
                int animation_id;
                int subtype;
                int special_resource_index;

                runtime_entity->resource_index =
                    runtime_entity->saved_resource_index;
                runtime_entity->saved_presentation_flag_bits.has_saved_resource =
                    0;
                subtype = runtime_entity->base.property_00a_bits.subtype;
                if ((u32)subtype <= 1 &&
                    (special_resource_index =
                         ((FieldSpecialResourceState *)(
                              (u8 *)runtime_entity + 0x592))
                             ->resource_index) != -1) {
                    if (((FieldSpecialResourceState *)(
                             (u8 *)runtime_entity + 0x592))
                            ->use_primary_resource_set == 0) {
                        resource_record = *(const u8 **)(
                            field_context +
                            FIELD_VM_SPECIAL_RESOURCE_RECORDS_OFFSET) +
                            (s16)special_resource_index *
                                FIELD_VM_RESOURCE_RECORD_SIZE;
                    } else {
                        int count = *(int *)(
                            field_context + FIELD_VM_RESOURCE_COUNTS_OFFSET);

                        if (special_resource_index < count) {
                            resource_record = *(const u8 **)(
                                field_context +
                                FIELD_VM_RESOURCE_RECORDS_OFFSET) +
                                special_resource_index *
                                    FIELD_VM_RESOURCE_RECORD_SIZE;
                        } else {
                            resource_record = 0;
                        }
                    }
                } else {
                    int resource_set =
                        runtime_entity->base.property_00a_bits.resource_set;
                    int count = *(int *)(
                        field_context + FIELD_VM_RESOURCE_COUNTS_OFFSET +
                        resource_set * sizeof(int));

                    if (runtime_entity->resource_index < count) {
                        resource_record = *(const u8 **)(
                            field_context + FIELD_VM_RESOURCE_RECORDS_OFFSET +
                            resource_set * sizeof(void *)) +
                            runtime_entity->resource_index *
                                FIELD_VM_RESOURCE_RECORD_SIZE;
                    } else {
                        resource_record = 0;
                    }
                }
                if (runtime_entity->saved_presentation_flag_bits.
                        has_saved_resource_animation != 0) {
                    runtime_entity->saved_presentation_flag_bits.
                        has_saved_resource_animation = 0;
                    animation_id = runtime_entity->saved_animation_id;
                } else {
                    animation_id = -1;
                }
                FieldEntity_RebindRendererResources(
                    runtime_entity, (const FieldPrimaryResource *)resource_record, 0, 0,
                    animation_id, 1, 0);
            } else if (runtime_entity->saved_presentation_flag_bits.
                           has_saved_resource_animation != 0) {
                runtime_entity->animation_id =
                    runtime_entity->saved_animation_id;
                runtime_entity->saved_presentation_flag_bits.
                    has_saved_resource_animation = 0;
                FieldEntity_SetResourceAnimation(
                    runtime_entity, runtime_entity->animation_id,
                    -1, 1, 1);
            }
            break;

        case FIELD_VM_WAIT_ENTITY_ANIMATION:
            if (runtime_entity->base_state_flag_bits.
                    animation_wait_enabled != 0 &&
                runtime_entity->render_object->state_flag_bits.
                    animation_active != 0 &&
                runtime_entity->render_object->state_flag_bits.
                    animation_suppressed == 0) {
                result = FieldVm_RetryCurrentCommand(
                    vm, state, command->opcode);
                break;
            }
            break;

        case FIELD_VM_SET_ENTITY_ANIMATION:
            if (arguments[2] != 0) {
                runtime_entity->saved_model_animation =
                    runtime_entity->render_object->animation_id;
                runtime_entity->saved_presentation_flag_bits.
                    has_saved_animation = 1;
            }
            runtime_entity->render_object->set_animation((u8)runtime_entity->animation_id,
                (s16)arguments[1], 1);
            break;

        case FIELD_VM_RESTORE_ENTITY_ANIMATION:
            if (runtime_entity->saved_presentation_flag_bits.
                    has_saved_animation != 0) {
                runtime_entity->saved_presentation_flag_bits.
                    has_saved_animation = 0;
                runtime_entity->render_object->set_animation((u8)runtime_entity->animation_id,
                    (s16)runtime_entity->saved_model_animation, 1);
            }
            break;

        case FIELD_VM_SET_ENTITY_BEHAVIOR_MODE:
            if (arguments[2] != 0) {
                runtime_entity->saved_presentation_flag_bits.
                    saved_behavior_mode =
                    runtime_entity->saved_presentation_flag_bits.
                        behavior_mode;
                runtime_entity->saved_presentation_flag_bits.
                    has_saved_behavior = 1;
            }
            FieldEntity_SetBehaviorMode(
                &runtime_entity->base, arguments[1]);
            break;

        case FIELD_VM_RESTORE_ENTITY_BEHAVIOR_STATE:
            if (runtime_entity->saved_presentation_flag_bits.
                    has_saved_behavior != 0) {
                FieldEntity_SetBehaviorMode(
                    &runtime_entity->base,
                    runtime_entity->saved_presentation_flag_bits.
                        saved_behavior_mode);
                runtime_entity->saved_presentation_flag_bits.
                    has_saved_behavior = 0;
            }
            if (runtime_entity->render_object->state_flag_bits.
                    behavior_state != 0) {
                runtime_entity->render_object->state_flag_bits.
                    behavior_state = 0;
                runtime_entity->render_object->state_flag_bits.
                    animation_suppressed = 0;
            }
            break;

        case FIELD_VM_SET_ENTITY_PALETTE_PROFILE:
            if (arguments[2] != 0) {
                runtime_entity->saved_presentation_state =
                    runtime_entity->presentation_state;
                runtime_entity->saved_presentation_flag_bits.
                    has_saved_palette_profile = 1;
            }
            FieldEntity_SetPaletteProfile(
                (FieldResourceContext *)field_context, runtime_entity,
                arguments[1]);
            break;

        case FIELD_VM_RESTORE_ENTITY_PALETTE_PROFILE:
            if (runtime_entity->saved_presentation_flag_bits.
                    has_saved_palette_profile != 0) {
                int palette_selector;

                runtime_entity->presentation_state =
                    runtime_entity->saved_presentation_state;
                runtime_entity->saved_presentation_flag_bits.
                    has_saved_palette_profile = 0;
                palette_selector =
                    runtime_entity->presentation_state & 0x0F;
                if ((runtime_entity->presentation_state & 0x80) != 0) {
                    palette_selector += 16;
                }
                FieldEntity_SetPaletteProfile(
                    (FieldResourceContext *)field_context, runtime_entity,
                    palette_selector);
            }
            break;

        case FIELD_VM_START_ENTITY_PALETTE_ANIMATION:
            if (arguments[1] == -1) {
                int palette_slot;

                for (palette_slot = 0; palette_slot < 8; palette_slot++) {
                    if (runtime_entity->render_object->components[palette_slot]
                        == 0) {
                        arguments[1] = palette_slot;
                    }
                }
            }
            if (arguments[1] != -1) {
                int palette_mode;

                runtime_entity->render_object->bind_palette_animation((s8)arguments[1],
                    (s16)arguments[2], 1);
                palette_mode = arguments[3] != 0 ? 2 : 0;
                runtime_entity->render_object->set_palette_animation_mode((s8)arguments[1], palette_mode);
                VM_WriteVariable(
                    command->result_variable, arguments[1], vm,
                    base_state);
            }
            break;

        case FIELD_VM_STOP_ENTITY_PALETTE_ANIMATION:
            runtime_entity->render_object->bind_palette_animation((s8)arguments[1], -1,
                1);
            break;

        case FIELD_VM_PAUSE_ENTITY_PALETTE_ANIMATION:
            runtime_entity->render_object->set_palette_animation_paused((s8)arguments[1],
                1);
            break;

        case FIELD_VM_RESUME_ENTITY_PALETTE_ANIMATION:
            runtime_entity->render_object->set_palette_animation_paused((s8)arguments[1],
                0);
            break;

        case FIELD_VM_REQUEST_ENTITY_BASE_PALETTE_RELOAD:
            runtime_entity->render_object->request_base_palette_reload();
            break;

        case FIELD_VM_WAIT_ENTITY_BASE_PALETTE_RELOAD:
            if (runtime_entity->render_object->transition != 0 &&
                runtime_entity->render_object->transition_active == 0) {
                result = FieldVm_RetryCurrentCommand(
                    vm, state, command->opcode);
                break;
            }
            break;

        case FIELD_VM_START_ENTITY_SCALING:
            FieldScaling_Start(
                runtime_entity, arguments[1],
                (s16)(arguments[2] / 16),
                (s16)(arguments[3] / 16),
                (s16)(arguments[4] / 16),
                (s16)(arguments[5] / 16), 0);
            break;

        case FIELD_VM_START_ENTITY_TIMED_SCALING:
            FieldScaling_StartTimed(
                runtime_entity, arguments[1],
                (s16)(arguments[2] / 16),
                (s16)(arguments[3] / 16), arguments[4],
                arguments[5], 0);
            break;

        case FIELD_VM_WAIT_ENTITY_SCALING:
            if (runtime_entity->transform_flag_bits.scaling_active != 0) {
                result = FieldVm_RetryCurrentCommand(
                    vm, state, command->opcode);
                break;
            }
            break;

        case FIELD_VM_STOP_ENTITY_SCALING:
            FieldScaling_Stop(runtime_entity, 0, 0);
            break;

        case FIELD_VM_START_ENTITY_ROTATION:
            func_ov000_020a4214(
                entity, arguments[1],
                FieldVm_DegreesToTurn16(arguments[2]),
                FieldVm_DegreesToTurn16(arguments[3]),
                arguments[4],
                arguments[5] != 0, 0);
            break;

        case FIELD_VM_START_ENTITY_TIMED_ROTATION:
            FieldRotation_StartTimed(
                runtime_entity, arguments[1],
                FieldVm_DegreesToTurn16(arguments[2]),
                arguments[3], arguments[4],
                arguments[5] != 0, 0);
            break;

        case FIELD_VM_WAIT_ENTITY_ROTATION:
            if (runtime_entity->transform_flag_bits.rotation_active != 0) {
                result = FieldVm_RetryCurrentCommand(
                    vm, state, command->opcode);
                break;
            }
            break;

        case FIELD_VM_STOP_ENTITY_ROTATION:
            FieldRotation_Stop(runtime_entity, 0, 0);
            break;

        case FIELD_VM_SET_ENTITY_LOCOMOTION_PARAMETERS:
            FieldEntity_SetLocomotionParameters(
                runtime_entity, arguments[1], arguments[2],
                arguments[3], -arguments[4],
                -arguments[5], arguments[6]);
            break;

        case FIELD_VM_START_ENTITY_MOVEMENT:
            if (FieldVm_IsTwoDimensionalEntity(runtime_entity)) {
                FieldLinear_Start(
                    runtime_entity, arguments[1],
                    arguments[2] << FX32B_INT,
                    arguments[3] << FX32B_INT,
                    arguments[5], arguments[6],
                    arguments[7], -arguments[8],
                    arguments[11] != 0, 0);
            } else {
                FieldLinear3D_Start(
                    runtime_entity, arguments[1],
                    arguments[2] << FX32B_INT,
                    arguments[3] << FX32B_INT,
                    arguments[4] << FX32B_INT,
                    arguments[5], arguments[6],
                    arguments[7], -arguments[8],
                    arguments[9], arguments[10],
                    arguments[11] != 0, 0);
            }
            break;

        case FIELD_VM_START_ENTITY_TIMED_MOVEMENT:
            if (FieldVm_IsTwoDimensionalEntity(runtime_entity)) {
                FieldLinear_StartTimed(
                    runtime_entity, arguments[1],
                    arguments[2] << FX32B_INT,
                    arguments[3] << FX32B_INT,
                    arguments[5], arguments[8] != 0, 0);
            } else {
                FieldLinear3D_StartTimed(
                    runtime_entity, arguments[1],
                    arguments[2] << FX32B_INT,
                    arguments[3] << FX32B_INT,
                    arguments[4] << FX32B_INT,
                    arguments[5], arguments[6],
                    arguments[7], arguments[8] != 0, 0);
            }
            break;

        case FIELD_VM_START_ENTITY_MOVEMENT_RELATIVE_TO_ENTITY:
            if (FieldVm_IsTwoDimensionalEntity(runtime_entity)) {
                FieldLinear_StartFollowing(
                    runtime_entity,
                    (FieldRuntimeEntity *)FieldVm_GetEntityByIndex(field_context, arguments[1]),
                    arguments[2] << FX32B_INT,
                    arguments[3] << FX32B_INT,
                    arguments[5], arguments[8] != 0, 0);
            } else {
                FieldLinear3D_StartFollowing(
                    runtime_entity,
                    (FieldRuntimeEntity *)FieldVm_GetEntityByIndex(field_context, arguments[1]),
                    arguments[2] << FX32B_INT,
                    arguments[3] << FX32B_INT,
                    arguments[4] << FX32B_INT,
                    arguments[5], arguments[6],
                    arguments[7], arguments[8] != 0, 0);
            }
            break;

        case FIELD_VM_START_ENTITY_TIMED_MOVEMENT_RELATIVE_TO_ENTITY:
            if (FieldVm_IsTwoDimensionalEntity(runtime_entity)) {
                FieldLinear_StartTimedFollowing(
                    runtime_entity,
                    (FieldRuntimeEntity *)FieldVm_GetEntityByIndex(field_context, arguments[1]),
                    arguments[2] << FX32B_INT,
                    arguments[3] << FX32B_INT,
                    arguments[5], arguments[8] != 0, 0);
            } else {
                FieldLinear3D_StartTimedFollowing(
                    runtime_entity,
                    (FieldRuntimeEntity *)FieldVm_GetEntityByIndex(field_context, arguments[1]),
                    arguments[2] << FX32B_INT,
                    arguments[3] << FX32B_INT,
                    arguments[4] << FX32B_INT,
                    arguments[5], arguments[6],
                    arguments[7], arguments[8] != 0, 0);
            }
            break;

        case FIELD_VM_START_ENTITY_ORBIT_AROUND_POINT:
            if (FieldVm_IsTwoDimensionalEntity(runtime_entity)) {
                FieldOrbit_StartAroundPoint(
                    runtime_entity,
                    (arguments[1] >> 3) & 1,
                    arguments[2] << FX32B_INT,
                    arguments[3] << FX32B_INT,
                    (arguments[1] >> 2) & 1,
                    FieldVm_DegreesToTurn16(arguments[5]),
                    arguments[6], arguments[7],
                    arguments[8], -arguments[9],
                    arguments[10], arguments[11],
                    arguments[14] != 0, 0);
            } else {
                func_ov000_020b2e9c(
                    &runtime_entity->base,
                    (arguments[1] >> 3) & 1,
                    arguments[2] << FX32B_INT,
                    arguments[3] << FX32B_INT,
                    arguments[4] << FX32B_INT,
                    (arguments[1] >> 2) & 1,
                    FieldVm_DegreesToTurn16(arguments[5]),
                    arguments[6], arguments[7],
                    arguments[8], -arguments[9],
                    arguments[1] & 3, arguments[10],
                    arguments[11], arguments[12],
                    arguments[13], arguments[14] != 0, 0);
            }
            break;

        case FIELD_VM_START_ENTITY_TIMED_ORBIT_AROUND_POINT:
            if (FieldVm_IsTwoDimensionalEntity(runtime_entity)) {
                FieldOrbit_StartTimedAroundPoint(
                    runtime_entity,
                    (arguments[1] >> 3) & 1,
                    arguments[2] << FX32B_INT,
                    arguments[3] << FX32B_INT,
                    (arguments[1] >> 2) & 1,
                    FieldVm_DegreesToTurn16(arguments[5]),
                    arguments[6], arguments[7],
                    arguments[8], arguments[11] != 0, 0);
            } else {
                FieldOrbit3D_StartTimedAroundPoint(
                    runtime_entity,
                    (arguments[1] >> 3) & 1,
                    arguments[2] << FX32B_INT,
                    arguments[3] << FX32B_INT,
                    arguments[4] << FX32B_INT,
                    (arguments[1] >> 2) & 1,
                    FieldVm_DegreesToTurn16(arguments[5]),
                    arguments[6], arguments[1] & 3,
                    arguments[7], arguments[8],
                    arguments[9], arguments[10],
                    arguments[11] != 0, 0);
            }
            break;

        case FIELD_VM_START_ENTITY_ORBIT_AROUND_ENTITY:
            if (FieldVm_IsTwoDimensionalEntity(runtime_entity)) {
                FieldOrbit_StartAroundEntity(
                    runtime_entity,
                    (FieldRuntimeEntity *)FieldVm_GetEntityByIndex(field_context, arguments[1]),
                    arguments[2] << FX32B_INT,
                    arguments[3] << FX32B_INT,
                    (arguments[5] >> 2) & 1,
                    FieldVm_DegreesToTurn16(arguments[6]),
                    arguments[7], arguments[8],
                    arguments[9], arguments[12] != 0, 0);
            } else {
                func_ov000_020b295c(
                    &runtime_entity->base,
                    FieldVm_GetEntityByIndex(field_context, arguments[1]),
                    arguments[2] << FX32B_INT,
                    arguments[3] << FX32B_INT,
                    arguments[4] << FX32B_INT,
                    (arguments[5] >> 2) & 1,
                    FieldVm_DegreesToTurn16(arguments[6]),
                    arguments[7], arguments[5] & 3,
                    arguments[8], arguments[9],
                    arguments[10], arguments[11],
                    arguments[12] != 0, 0);
            }
            break;

        case FIELD_VM_START_ENTITY_TIMED_ORBIT_AROUND_ENTITY:
            if (FieldVm_IsTwoDimensionalEntity(runtime_entity)) {
                FieldOrbit_StartTimedAroundEntity(
                    runtime_entity,
                    (FieldRuntimeEntity *)FieldVm_GetEntityByIndex(field_context, arguments[1]),
                    arguments[2] << FX32B_INT,
                    arguments[3] << FX32B_INT,
                    (arguments[5] >> 2) & 1,
                    FieldVm_DegreesToTurn16(arguments[6]),
                    arguments[7], arguments[8],
                    arguments[9], arguments[12] != 0, 0);
            } else {
                func_ov000_020b26ac(
                    &runtime_entity->base,
                    FieldVm_GetEntityByIndex(field_context, arguments[1]),
                    arguments[2] << FX32B_INT,
                    arguments[3] << FX32B_INT,
                    arguments[4] << FX32B_INT,
                    (arguments[5] >> 2) & 1,
                    FieldVm_DegreesToTurn16(arguments[6]),
                    arguments[7], arguments[5] & 3,
                    arguments[8], arguments[9],
                    arguments[10], arguments[11],
                    arguments[12] != 0, 0);
            }
            break;

        case FIELD_VM_WAIT_ENTITY_MOVEMENT:
            if (runtime_entity->linear_controller.bits.active_axes != 0 ||
                runtime_entity->orbit_controller.bits.active != 0) {
                result = FieldVm_RetryCurrentCommand(
                    vm, state, command->opcode);
                break;
            }
            break;

        case FIELD_VM_CANCEL_ENTITY_MOVEMENT:
            entity->cancel_linear_movement(0, 0);
            entity->cancel_orbit_movement(0, 0);
            break;

        case FIELD_VM_SET_ENTITY_POSITION:
            if (FieldVm_IsTwoDimensionalEntity(runtime_entity)) {
                arguments[2] <<= FX32B_INT;
                arguments[3] <<= FX32B_INT;
                if (arguments[1] != 0) {
                    arguments[2] += runtime_entity->position_x;
                    arguments[3] += runtime_entity->position_y;
                }
                FieldEntity2D_SetPosition(
                    runtime_entity, arguments[2],
                    arguments[3]);
            } else {
                arguments[2] <<= FX32B_INT;
                arguments[3] <<= FX32B_INT;
                arguments[4] <<= FX32B_INT;
                if (arguments[1] != 0) {
                    arguments[2] += runtime_entity->position_x;
                    arguments[3] += runtime_entity->position_y;
                    arguments[4] += runtime_entity->position_z;
                }
                FieldEntity3D_SetPosition(
                    runtime_entity, arguments[2],
                    arguments[3], arguments[4]);
                if (FieldVm_GetSpecialPartyState(field_context)
                            ->camera_focus_enabled != 0 &&
                    *(s8 *)(field_context +
                            FIELD_VM_SPECIAL_PARTY_ENTITY_ID_OFFSET) != -1 &&
                    *(s8 *)(field_context +
                            FIELD_VM_SPECIAL_PARTY_ENTITY_ID_OFFSET) ==
                        *(u8 *)((u8 *)runtime_entity + 4)) {
                    FieldArea_CenterCameraOnEntity(
                        (FieldAreaContext *)field_context, runtime_entity, 0, 0);
                }
            }
            break;

        case FIELD_VM_SET_ENTITY_FACING_DIRECTION:
            FieldEntity_SetFacingDirection(
                runtime_entity, arguments[1], arguments[2], 1);
            break;

        case FIELD_VM_SPAWN_ENTITY_EFFECT_SPRITE:
            FieldEffect_StartForEntity(
                field_context, (FieldRuntimeEntity *)entity, -1,
                arguments[1], arguments[2],
                (s16)arguments[3],
                (s16)arguments[4], arguments[5],
                arguments[6] != 0);
            break;

        case FIELD_VM_REMOVE_ENTITY_EFFECT_SPRITE:
            FieldEffect_StopForEntity(field_context, (FieldRuntimeEntity *)entity);
            break;

        case FIELD_VM_WAIT_ENTITY_EFFECT_SPRITE:
            if (func_ov000_0207133c(field_context, entity)) {
                result = FieldVm_RetryCurrentCommand(
                    vm, state, command->opcode);
                break;
            }
            break;
        }
        break;

    case FIELD_VM_SET_ENTITY_TURN_TO_INTERACTOR_ENABLED:
    case FIELD_VM_SET_ENTITY_GROUND_TRACKING:
    case FIELD_VM_SET_ENTITY_ALTERNATE_COLLISION_FACES_ENABLED:
    case FIELD_VM_SET_ENTITY_NAVIGATION_OBSTACLE_IGNORED:
    case FIELD_VM_SET_ENTITY_LINKED_CONTACT_MODE:
    case FIELD_VM_SET_ENTITY_CONTACT_DIRECTION_FILTER:
    case FIELD_VM_CONFIGURE_ENTITY_SHADOW:
    case FIELD_VM_SET_ENTITY_MAP_SYNC_AXES:
    case FIELD_VM_SET_ENTITY_NAVIGATION_COLLISION_BOUNDS:
    case FIELD_VM_RESTORE_ENTITY_NAVIGATION_COLLISION_BOUNDS:
    case FIELD_VM_SET_ENTITY_COLLISION_RESPONSE_CHANNELS:
    case FIELD_VM_SET_ENTITY_COLLISION_RESPONSE_CHANNELS_MASKED:
    case FIELD_VM_RESTORE_ENTITY_COLLISION_RESPONSE_CHANNELS:
    case FIELD_VM_SET_ENTITY_BODY_COLLISION_BOUNDS:
    case FIELD_VM_RESTORE_ENTITY_BODY_COLLISION_BOUNDS:
    case FIELD_VM_SET_ENTITY_RESERVED_COLLISION_FLAG:
    case FIELD_VM_SET_ENTITY_CATEGORY_COLLISION_POLICY:
    case FIELD_VM_SET_ENTITY_DEFAULT_VERTICAL_LAUNCH_VELOCITY:
    case FIELD_VM_SET_ENTITY_DEFAULT_GRAVITY:
    case FIELD_VM_SET_ENTITY_TERMINAL_FALL_VELOCITY:
    case FIELD_VM_SET_ENTITY_SHADOW_SUPPORT_ENABLED:
    case FIELD_VM_START_ENTITY_VERTICAL_MOTION:
    case FIELD_VM_START_ENTITY_VERTICAL_MOTION_TO_HEIGHT:
    case FIELD_VM_WAIT_ENTITY_VERTICAL_MOTION:
    case FIELD_VM_STOP_ENTITY_VERTICAL_MOTION:
    case FIELD_VM_SET_ENTITY_ROAMING_BOUNDS:
    case FIELD_VM_ADD_ENTITY_ROAMING_PROFILE:
    case FIELD_VM_SET_ENTITY_ROAMING_BOUNDARY_CLAMP_ENABLED:
    case FIELD_VM_START_ENTITY_RANDOM_ROAMING:
    case FIELD_VM_STOP_ENTITY_RANDOM_ROAMING:
    case FIELD_VM_PAUSE_ENTITY_RANDOM_ROAMING:
    case FIELD_VM_RESUME_ENTITY_RANDOM_ROAMING:
    case FIELD_VM_CLEAR_ENTITY_ROAMING_PROFILES:
    case FIELD_VM_LOAD_ENTITY_WAYPOINT_PATH:
    case FIELD_VM_START_ENTITY_WAYPOINT_PATH:
    case FIELD_VM_STOP_ENTITY_WAYPOINT_PATH:
    case FIELD_VM_PAUSE_ENTITY_WAYPOINT_PATH:
    case FIELD_VM_RESUME_ENTITY_WAYPOINT_PATH:
    case FIELD_VM_CLEAR_ENTITY_WAYPOINT_PATH:
    case FIELD_VM_FACE_ENTITY_TOWARD_ENTITY:
    case FIELD_VM_SET_FIELD_BLOCK_IDLE_BOBBING_ENABLED:
    case FIELD_VM_SET_FIELD_BLOCK_BOUNCE_CONTROLLER_ENABLED:
    case FIELD_VM_WAIT_FIELD_BLOCK_BOUNCE:
    case FIELD_VM_SET_ENEMY_JUMP_FIRST_STRIKE_ENABLED:
    case FIELD_VM_SET_ENEMY_SPIKED_JUMP_RESPONSE:
    case FIELD_VM_SET_ENEMY_SPECIAL_CONTACT_REMOVAL_ENABLED:
    case FIELD_VM_SET_ENEMY_IMMEDIATE_BATTLE_REMOVAL_ENABLED:
        entity = FieldScript_ResolveEntity(
            runtime, state, arguments[0]);
        runtime_entity = FieldVm_GetRuntimeEntity(entity);
        switch (command->opcode) {
        case FIELD_VM_SET_ENTITY_TURN_TO_INTERACTOR_ENABLED:
            runtime_entity->field_state_flag_bits.turn_to_interactor =
                arguments[1] != 0;
            break;

        case FIELD_VM_SET_ENTITY_GROUND_TRACKING: {
            int enabled = arguments[1] != 0;

            runtime_entity->field_state_flag_bits.track_ground =
                enabled;
            if (enabled) {
                runtime_entity->runtime_flags |=
                    FIELD_ENTITY_VERTICAL_SYNC_DIRTY;
            }
            break;
        }

        case FIELD_VM_SET_ENTITY_ALTERNATE_COLLISION_FACES_ENABLED:
            runtime_entity->runtime_flag_bits.alternate_collision_faces =
                arguments[1] != 0;
            break;

        case FIELD_VM_SET_ENTITY_NAVIGATION_OBSTACLE_IGNORED:
            runtime_entity->field_state_flag_bits.ignore_navigation_obstacle =
                arguments[1] != 0;
            break;
        case FIELD_VM_SET_ENTITY_LINKED_CONTACT_MODE:
            runtime_entity->field_state_flags =
                (runtime_entity->field_state_flags &
                 ~FIELD_ENTITY_CONTACT_MODE_MASK) |
                (arguments[1] & FIELD_ENTITY_CONTACT_MODE_MASK);
            break;

        case FIELD_VM_SET_ENTITY_CONTACT_DIRECTION_FILTER:
            FieldVm_SetContactDirectionFilter(
                runtime_entity, arguments[1]);
            break;
        case FIELD_VM_CONFIGURE_ENTITY_SHADOW:
            runtime_entity->field_state_flag_bits.shadow_enabled =
                arguments[1];
            if (arguments[2] == -1) {
                if (runtime_entity->field_state_flag_bits.shadow_enabled != 0 &&
                    runtime_entity->field_state_flag_bits.shadow_style == 0) {
                    runtime_entity->field_state_flag_bits.shadow_style = 1;
                }
            } else {
                runtime_entity->field_state_flag_bits.shadow_style =
                    arguments[2];
            }
            break;

        case FIELD_VM_SET_ENTITY_MAP_SYNC_AXES: {
            int horizontal = arguments[1];
            int vertical = arguments[2];

            if (horizontal != -1) {
                runtime_entity->runtime_flags =
                    (runtime_entity->runtime_flags &
                     ~FIELD_ENTITY_SYNC_HORIZONTAL) |
                    (horizontal & 1);
            }
            if (vertical != -1) {
                runtime_entity->runtime_flags =
                    (runtime_entity->runtime_flags &
                     ~FIELD_ENTITY_SYNC_VERTICAL) |
                    ((vertical & 1) << 1);
            }
            if (arguments[1] == 1) {
                runtime_entity->runtime_flags |=
                    FIELD_ENTITY_HORIZONTAL_SYNC_DIRTY;
            }
            if (arguments[2] == 1) {
                runtime_entity->runtime_flags |=
                    FIELD_ENTITY_VERTICAL_SYNC_DIRTY;
            }
            break;
        }

        case FIELD_VM_SET_ENTITY_NAVIGATION_COLLISION_BOUNDS:
            FieldVm_SetNavigationBounds(runtime_entity, arguments);
            break;

        case FIELD_VM_RESTORE_ENTITY_NAVIGATION_COLLISION_BOUNDS:
            FieldEntity_LoadNavigationBounds(runtime_entity);
            break;

        case FIELD_VM_SET_ENTITY_COLLISION_RESPONSE_CHANNELS:
            entity->set_collision_response_channels(arguments[1], arguments[2],
                arguments[3], arguments[4],
                arguments[5]);
            break;

        case FIELD_VM_SET_ENTITY_COLLISION_RESPONSE_CHANNELS_MASKED:
            entity->set_collision_response_channels_masked((u16)arguments[1], arguments[2]);
            break;

        case FIELD_VM_RESTORE_ENTITY_COLLISION_RESPONSE_CHANNELS:
            entity->restore_collision_response_channels();
            break;

        case FIELD_VM_SET_ENTITY_BODY_COLLISION_BOUNDS:
            FieldVm_SetBodyBounds(runtime_entity, arguments);
            break;

        case FIELD_VM_RESTORE_ENTITY_BODY_COLLISION_BOUNDS:
            FieldEntity_LoadBodyBounds((FieldRuntimeEntity *)entity);
            break;

        case FIELD_VM_SET_ENTITY_RESERVED_COLLISION_FLAG: {
            int enabled = arguments[2] != 0;

            switch (arguments[1]) {
            case 0:
                runtime_entity->collision_state_flag_bits.reserved_0 =
                    enabled;
                break;
            case 1:
                runtime_entity->collision_state_flag_bits.reserved_1 =
                    enabled;
                break;
            }
            break;
        }

        case FIELD_VM_SET_ENTITY_CATEGORY_COLLISION_POLICY:
            FieldVm_SetCollisionPolicy(runtime_entity, arguments);
            break;

        /*
         * Opcodes 0x060 and 0x061 enter the other entity-family switch in
         * the outer table. These duplicate handlers are therefore dead, but
         * they are present in the original second inner switch.
         */
        case FIELD_VM_SET_ENTITY_RENDER_LAYER: {
            u8 render_layer = (u8)arguments[1];

            if (runtime_entity->render_object != 0) {
                runtime_entity->render_object->sort_key_bits.layer =
                    render_layer;
            }
            break;
        }

        case FIELD_VM_LEGACY_NOOP_061:
            runtime_entity->field_state_flag_bits.body_collision_enabled =
                arguments[1];
            break;

        case FIELD_VM_SET_ENTITY_DEFAULT_VERTICAL_LAUNCH_VELOCITY:
            runtime_entity->default_vertical_launch_velocity =
                FieldVm_DecodeWideArgument(command, 1);
            break;

        case FIELD_VM_SET_ENTITY_DEFAULT_GRAVITY:
            runtime_entity->default_gravity =
                FieldVm_DecodeWideArgument(command, 1);
            break;

        case FIELD_VM_SET_ENTITY_TERMINAL_FALL_VELOCITY:
            runtime_entity->terminal_fall_velocity =
                FieldVm_DecodeWideArgument(command, 1);
            break;

        case FIELD_VM_SET_ENTITY_SHADOW_SUPPORT_ENABLED:
            runtime_entity->field_state_flag_bits.shadow_support_enabled =
                arguments[1];
            break;

        case FIELD_VM_START_ENTITY_VERTICAL_MOTION:
            FieldVertical_Start(
                runtime_entity, FieldVm_DecodeWideArgument(command, 1),
                FieldVm_DecodeWideArgument(command, 3), -1);
            break;

        case FIELD_VM_START_ENTITY_VERTICAL_MOTION_TO_HEIGHT:
            FieldVertical_StartToHeight(
                runtime_entity, arguments[1] << FX32B_INT,
                FieldVm_DecodeWideArgument(command, 2), -1);
            break;

        case FIELD_VM_WAIT_ENTITY_VERTICAL_MOTION:
            if (runtime_entity->field_state_flag_bits.vertical_motion_active !=
                0) {
                result = FieldVm_RetryCurrentCommand(
                    vm, state, command->opcode);
                break;
            }
            break;

        case FIELD_VM_STOP_ENTITY_VERTICAL_MOTION:
            FieldVertical_Stop(runtime_entity);
            break;

        case FIELD_VM_SET_ENTITY_ROAMING_BOUNDS:
            FieldRoaming_SetBounds((FieldRuntimeEntity *)entity, arguments[1], arguments[2],
                arguments[3], arguments[4]);
            break;

        case FIELD_VM_ADD_ENTITY_ROAMING_PROFILE:
            FieldRoaming_AddOption((FieldRuntimeEntity *)entity, (const FieldRoamingOptionInput *)(state->vm_state.script + arguments[1] + 2),
                -1);
            break;

        case FIELD_VM_SET_ENTITY_ROAMING_BOUNDARY_CLAMP_ENABLED:
            runtime_entity->roaming_flag_bits.clamp_to_boundary =
                arguments[1];
            break;

        case FIELD_VM_START_ENTITY_RANDOM_ROAMING:
            FieldRoaming_ChooseDestination((FieldRuntimeEntity *)entity);
            break;

        case FIELD_VM_STOP_ENTITY_RANDOM_ROAMING:
            FieldRoaming_Stop((FieldRuntimeEntity *)entity);
            break;

        case FIELD_VM_PAUSE_ENTITY_RANDOM_ROAMING:
            FieldRoaming_Pause((FieldRuntimeEntity *)entity);
            break;

        case FIELD_VM_RESUME_ENTITY_RANDOM_ROAMING:
            FieldRoaming_Resume((FieldRuntimeEntity *)entity);
            break;

        case FIELD_VM_CLEAR_ENTITY_ROAMING_PROFILES:
            FieldRoaming_ClearOptions((FieldRuntimeEntity *)entity, -1);
            break;

        case FIELD_VM_LOAD_ENTITY_WAYPOINT_PATH:
            {
                int path_offset = arguments[1];
                int path_payload_size;

                script = state->vm_state.script;
                path_payload_size =
                    *(const s32 *)(script + path_offset) - 4;

                func_ov000_020b1a24(
                    entity, script + path_offset + 2,
                    path_payload_size / 2);
            }
            break;

        case FIELD_VM_START_ENTITY_WAYPOINT_PATH:
            FieldPath_Start((FieldRuntimeEntity *)entity);
            break;

        case FIELD_VM_STOP_ENTITY_WAYPOINT_PATH:
            FieldRoaming_Stop((FieldRuntimeEntity *)entity);
            break;

        case FIELD_VM_PAUSE_ENTITY_WAYPOINT_PATH:
            FieldRoaming_Pause((FieldRuntimeEntity *)entity);
            break;

        case FIELD_VM_RESUME_ENTITY_WAYPOINT_PATH:
            FieldRoaming_Resume((FieldRuntimeEntity *)entity);
            break;

        case FIELD_VM_CLEAR_ENTITY_WAYPOINT_PATH:
            FieldPath_Clear((FieldRuntimeEntity *)entity);
            break;

        case FIELD_VM_FACE_ENTITY_TOWARD_ENTITY:
            FieldEntity_FaceTarget(
                runtime_entity, (FieldRuntimeEntity *)FieldVm_GetEntityByIndex(field_context, arguments[1]));
            break;

        case FIELD_VM_SET_FIELD_BLOCK_IDLE_BOBBING_ENABLED:
            FieldVariableEntity_SetBobbing((FieldVariableEntity *)entity, arguments[1] != 0);
            break;

        case FIELD_VM_SET_FIELD_BLOCK_BOUNCE_CONTROLLER_ENABLED:
            FieldVariableEntity_SetEnabled((FieldVariableEntity *)entity, arguments[1] != 0);
            break;

        case FIELD_VM_WAIT_FIELD_BLOCK_BOUNCE:
            if (FieldVm_GetInteractionFlags(runtime_entity)
                    ->block_bounce_state != -1) {
                result = FieldVm_RetryCurrentCommand(
                    vm, state, command->opcode);
                break;
            }
            break;

        case FIELD_VM_SET_ENEMY_JUMP_FIRST_STRIKE_ENABLED:
            FieldVm_GetInteractionFlags(runtime_entity)->
                jump_first_strike_enabled = arguments[1] != 0;
            break;

        case FIELD_VM_SET_ENEMY_SPIKED_JUMP_RESPONSE:
            FieldVm_GetInteractionFlags(runtime_entity)->spiked_jump_response =
                arguments[1] != 0;
            break;

        case FIELD_VM_SET_ENEMY_SPECIAL_CONTACT_REMOVAL_ENABLED:
            FieldVm_GetInteractionFlags(runtime_entity)->
                remove_after_special_contact = arguments[1] != 0;
            break;

        case FIELD_VM_SET_ENEMY_IMMEDIATE_BATTLE_REMOVAL_ENABLED:
            FieldVm_GetInteractionFlags(runtime_entity)->
                remove_immediately_for_battle = arguments[1] != 0;
            break;
        }
        break;

    case FIELD_VM_WAIT_MATCHING_ENTITY_SCRIPTS:
    case FIELD_VM_STOP_MATCHING_ENTITY_SCRIPTS:
    case FIELD_VM_PAUSE_MATCHING_ENTITY_SCRIPTS:
    case FIELD_VM_RESUME_MATCHING_ENTITY_SCRIPTS:
        FieldVm_VisitMatchingEntityScripts(
            runtime, state, arguments[0], command->opcode, &result);
        break;

    case FIELD_VM_LEGACY_NOOP_064:
        /* The outer jump table never enters the dead inner 0x064 handler. */
        break;

    case FIELD_VM_CHANGE_FIELD_ROOM_FOR_PARTY:
        if (arguments[0] == -1) {
            int active_side = FieldVm_GetActivePartySide(party_manager);
            u8 *active_controller = FieldVm_GetPartyController(
                party_manager, active_side);
            int controller_screen =
                FieldVm_GetPartyControllerFlags(active_controller)
                    ->field_screen;
            int field_screen =
                FieldVm_GetSpecialPartyState(field_context)->field_screen;

            if (field_screen == controller_screen) {
                arguments[0] = active_side;
            }
        }
        FieldArea_QueuePartyRoomChange(
            (FieldAreaContext *)field_context, arguments[0],
            arguments[1], (s16)arguments[2],
            (s16)arguments[3], (s16)arguments[4],
            arguments[5], arguments[6] != 0,
            (s16)arguments[7], arguments[8], 0,
            arguments[9] != 0, -1, 1);
        result = SCRIPT_VM_RETURNED;
        break;

    case FIELD_VM_CHANGE_FIELD_ROOM_FOR_BOTH_PARTIES:
        FieldArea_QueueBothPartyRoomChange(
            (FieldAreaContext *)field_context, arguments[0],
            (s16)arguments[1], (s16)arguments[2],
            (s16)arguments[3], arguments[4],
            (s16)arguments[5], (s16)arguments[6],
            (s16)arguments[7], arguments[8],
            (s16)arguments[9], arguments[10], 0,
            arguments[11] != 0, -1, 1);
        result = SCRIPT_VM_RETURNED;
        break;

    case FIELD_VM_CHANGE_FIELD_ROOM:
        FieldArea_QueueRoomChange(
            (FieldAreaContext *)field_context, arguments[0],
            (s16)arguments[1], arguments[2], 0);
        result = SCRIPT_VM_RETURNED;
        break;

    case FIELD_VM_TRANSFER_PARTY_BETWEEN_FIELD_SCREENS: {
        int field_screen;
        if (arguments[0] == -1) {
            arguments[0] =
                FieldVm_GetActivePartySide(party_manager);
        }
        party_controller = FieldVm_GetPartyController(
            party_manager, arguments[0]);
        field_screen = FieldVm_GetSpecialPartyState(field_context)->field_screen;
        if (field_screen ==
            FieldVm_GetPartyControllerFlags(party_controller)->field_screen) {
            func_ov000_0209f644(
                party_manager, arguments[0],
                arguments[1], arguments[2],
                arguments[3], arguments[4],
                arguments[5], arguments[6] != 0,
                (s16)arguments[7], arguments[8],
                arguments[9] != 0,
                arguments[10] != 0);
        }
        break;
    }

    case FIELD_VM_ENSURE_PARTY_IN_CURRENT_FIELD:
        if (arguments[0] == -1) {
            arguments[0] =
                FieldVm_GetActivePartySide(party_manager) ^ 1;
        }
        FieldArea_EnsurePartyPresent(
            (FieldAreaContext *)field_context, arguments[0],
            (s16)arguments[1], (s16)arguments[2],
            (s16)arguments[3], arguments[4]);
        break;

    case FIELD_VM_ACTIVATE_FIELD_MAP_EVENT:
        FieldArea_ApplyNavigationChange(
            (FieldAreaContext *)field_context, arguments[0]);
        break;

    case FIELD_VM_START_MAP_TILE_ANIMATION:
        map_controller = *(FieldMapController **)(
            field_context + FIELD_VM_MAP_CONTROLLER_OFFSET);
        map_controller->start_tile_animation((u16)arguments[0]);
        break;

    case FIELD_VM_WAIT_MAP_TILE_ANIMATION:
        map_controller = *(FieldMapController **)(
            field_context + FIELD_VM_MAP_CONTROLLER_OFFSET);
        if (*(u8 *)((u8 *)map_controller + 0x670 +
                    12 * arguments[0]) == 1) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_PAUSE_MAP_TILE_ANIMATION:
        map_controller = *(FieldMapController **)(
            field_context + FIELD_VM_MAP_CONTROLLER_OFFSET);
        map_controller->pause_tile_animation((u16)arguments[0]);
        break;

    case FIELD_VM_RESUME_MAP_TILE_ANIMATION:
        map_controller = *(FieldMapController **)(
            field_context + FIELD_VM_MAP_CONTROLLER_OFFSET);
        map_controller->resume_tile_animation((u16)arguments[0]);
        break;

    case FIELD_VM_SET_FIELD_BG_LAYERS_ENABLED: {
        u32 layer_mask;
        int layer;
        int screen =
            FieldVm_GetSpecialPartyState(field_context)->field_screen;
        u32 display_control = screen == 0
            ? *(volatile u32 *)0x04000000
            : *(volatile u32 *)0x04001000;
        layer_mask = (display_control >> 8) & 0xF;

        for (layer = 0; layer < 4; layer++) {
            if (arguments[layer] == 1) {
                layer_mask |= 1 << layer;
            } else if (arguments[layer] == 0) {
                layer_mask &= ~(1 << layer);
            }
        }
        if (screen == 0) {
            *(volatile u32 *)0x04000000 =
                (*(volatile u32 *)0x04000000 & 0xFFFFE0FF) |
                ((layer_mask | 0x10) << 8);
        } else {
            *(volatile u32 *)0x04001000 =
                (*(volatile u32 *)0x04001000 & 0xFFFFE0FF) |
                ((layer_mask | 0x10) << 8);
        }
        break;
    }

    case FIELD_VM_START_CAMERA_PROFILED_MOVEMENT:
        FieldArea_MoveCameraWithProfile(
            (FieldAreaContext *)field_context, arguments[0],
            arguments[1] << FX32B_INT,
            arguments[2] << FX32B_INT,
            arguments[3], arguments[4],
            arguments[5], -arguments[6],
            arguments[7] != 0,
            arguments[8] != 0, 0);
        *(u16 *)(field_context +
                 FIELD_VM_CAMERA_TARGET_FLAGS_OFFSET) &= ~3;
        break;

    case FIELD_VM_START_CAMERA_TIMED_MOVEMENT:
        FieldArea_MoveCameraForDuration(
            (FieldAreaContext *)field_context, arguments[0],
            arguments[1] << FX32B_INT,
            arguments[2] << FX32B_INT,
            arguments[3], arguments[4] != 0,
            arguments[5] != 0, 0);
        *(u16 *)(field_context +
                 FIELD_VM_CAMERA_TARGET_FLAGS_OFFSET) &= ~3;
        break;

    case FIELD_VM_START_CAMERA_PROFILED_ENTITY_TRACKING:
        FieldArea_TrackEntityAtSpeed(
            (FieldAreaContext *)field_context,
            (FieldRuntimeEntity *)FieldVm_GetEntityByIndex(field_context, arguments[0]),
            arguments[1] << FX32B_INT,
            arguments[2] << FX32B_INT,
            arguments[3], arguments[4] != 0,
            arguments[5] != 0, 0);
        *(u16 *)(field_context +
                 FIELD_VM_CAMERA_TARGET_FLAGS_OFFSET) &= ~3;
        break;

    case FIELD_VM_START_CAMERA_TIMED_ENTITY_TRACKING:
        FieldArea_TrackEntityForDuration(
            (FieldAreaContext *)field_context,
            (FieldRuntimeEntity *)FieldVm_GetEntityByIndex(field_context, arguments[0]),
            arguments[1] << FX32B_INT,
            arguments[2] << FX32B_INT,
            arguments[3], arguments[4] != 0,
            arguments[5] != 0, 0);
        *(u16 *)(field_context +
                 FIELD_VM_CAMERA_TARGET_FLAGS_OFFSET) &= ~3;
        break;

    case FIELD_VM_WAIT_CAMERA_MOVEMENT:
        if (((FieldTwoBitActivityFlags *)(field_context +
                FIELD_VM_CAMERA_MOVEMENT_FLAGS_OFFSET))->activity_mode != 0) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_STOP_CAMERA_MOVEMENT:
        FieldArea_StopCameraMotion((FieldAreaContext *)field_context, 0, 0);
        break;

    case FIELD_VM_START_BG_LAYER_PROFILED_SCROLL:
        FieldArea_MoveLayerWithProfile(
            (FieldAreaContext *)field_context, arguments[0],
            arguments[1] << FX32B_INT,
            arguments[2] << FX32B_INT,
            arguments[3], arguments[4],
            arguments[5], -arguments[6],
            arguments[7] != 0);
        break;

    case FIELD_VM_START_BG_LAYER_TIMED_SCROLL:
        FieldArea_MoveLayerForDuration(
            (FieldAreaContext *)field_context, arguments[0],
            arguments[1] << FX32B_INT,
            arguments[2] << FX32B_INT,
            arguments[3], arguments[4] != 0);
        break;

    case FIELD_VM_WAIT_BG_LAYER_SCROLL: {
        FieldBgScrollControllerState *controllers =
            *(FieldBgScrollControllerState **)(
            field_context + FIELD_VM_BG_SCROLL_CONTROLLERS_OFFSET);

        if (controllers[arguments[0]].flags.activity_mode != 0) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;
    }

    case FIELD_VM_STOP_BG_LAYER_SCROLL:
        FieldArea_StopLayerMotion((FieldAreaContext *)field_context, arguments[0]);
        break;

    case FIELD_VM_START_CAMERA_SHAKE:
        FieldCamera_StartShake(
            field_context, arguments[0],
            arguments[1] << FX32B_INT, arguments[2],
            arguments[3], arguments[4]);
        break;

    case FIELD_VM_WAIT_CAMERA_SHAKE:
        if (((FieldSingleActivityFlags16 *)(field_context +
                FIELD_VM_CAMERA_SHAKE_FLAGS_OFFSET))->active) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_STOP_CAMERA_SHAKE:
        FieldCamera_StopShake(field_context);
        break;

    case FIELD_VM_START_MAP_PALETTE_ANIMATION: {
        int palette_mode = arguments[1] != 0 ? 0x10 : 0;
        FieldMapController *controller = *(FieldMapController **)(
            field_context + FIELD_VM_MAP_CONTROLLER_OFFSET);

        controller->start_palette_animation((u8)arguments[0],
            palette_mode);
        break;
    }

    case FIELD_VM_RESET_ALL_MAP_PALETTE_ANIMATIONS:
        map_controller = *(FieldMapController **)(
            field_context + FIELD_VM_MAP_CONTROLLER_OFFSET);
        map_controller->reset_palette_animations();
        break;

    case FIELD_VM_STOP_MAP_PALETTE_ANIMATION:
        map_controller = *(FieldMapController **)(
            field_context + FIELD_VM_MAP_CONTROLLER_OFFSET);
        map_controller->stop_palette_animation((u8)arguments[0]);
        break;

    case FIELD_VM_RESUME_MAP_PALETTE_ANIMATION:
        map_controller = *(FieldMapController **)(
            field_context + FIELD_VM_MAP_CONTROLLER_OFFSET);
        map_controller->resume_palette_animation((u8)arguments[0]);
        break;

    case FIELD_VM_RESTORE_MAP_BASE_PALETTES:
        map_controller = *(FieldMapController **)(
            field_context + FIELD_VM_MAP_CONTROLLER_OFFSET);
        map_controller->restore_base_palettes();
        break;

    case FIELD_VM_WAIT_MAP_PALETTE_ANIMATIONS:
        map_controller = *(FieldMapController **)(
            field_context + FIELD_VM_MAP_CONTROLLER_OFFSET);
        if (map_controller->are_palette_animations_active()) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_START_MASTER_BRIGHTNESS_TRANSITION: {
        struct FieldControlContext {
            u8 unknown[FIELD_VM_FIELD_CONTROL_FLAGS_OFFSET];
            FieldControlFlags flags;
        };
        FieldControlContext *control_context = (FieldControlContext *)field_context;

        FieldArea_StartBrightness(
            (FieldAreaContext *)field_context, (s16)arguments[0],
            (s16)arguments[1], (u16)arguments[2]);
        if (FieldVm_GetSpecialPartyState(field_context)->field_screen == 0) {
            FieldArea_ApplyQueuedMusic((FieldAreaContext *)field_context);
        }
        if (control_context->flags.release_party_action) {
            FieldParty_BeginEntrySlide((FieldPartyController *)FieldVm_GetPartyController(
                party_manager, control_context->flags.selected_party_side));
            control_context->flags.release_party_action = 0;
        }
        break;
    }

    case FIELD_VM_WAIT_MASTER_BRIGHTNESS_TRANSITION:
        if (*(u16 *)(field_context +
                    FIELD_VM_BRIGHTNESS_FRAMES_OFFSET) != 0) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_PAUSE_MASTER_BRIGHTNESS_TRANSITION:
        if (FieldVm_GetBrightnessTransitionFlags(field_context)->active) {
            FieldVm_GetBrightnessTransitionFlags(field_context)->paused = 1;
        }
        break;

    case FIELD_VM_RESUME_MASTER_BRIGHTNESS_TRANSITION:
        if (FieldVm_GetBrightnessTransitionFlags(field_context)->active) {
            FieldVm_GetBrightnessTransitionFlags(field_context)->paused = 0;
        }
        break;

    case FIELD_VM_REVERSE_MASTER_BRIGHTNESS_TRANSITION:
        if (FieldVm_GetBrightnessTransitionFlags(field_context)->active) {
            FieldArea_ReverseBrightness((FieldAreaContext *)field_context);
        }
        break;

    case FIELD_VM_START_FIELD_PALETTE_ANIMATION:
        func_ov000_02075814(
            field_context, arguments[0],
            arguments[1], (u16)arguments[2],
            (u16)arguments[3], (u16)arguments[4],
            (u16)arguments[5], (u16)arguments[6],
            (u16)arguments[7], (s16)arguments[8],
            (u16)(((arguments[11] << 10) |
                   (arguments[10] << 5)) | arguments[9]));
        break;

    case FIELD_VM_WAIT_FIELD_PALETTE_ANIMATION:
        if (arguments[0] == -1) {
            int animation_slot;

            for (animation_slot = 0; animation_slot < 2;
                 animation_slot++) {
                if (FieldArea_AreEffectsActive((FieldAreaContext *)field_context, animation_slot)) {
                    result = FieldVm_RetryCurrentCommand(
                        vm, state, command->opcode);
                    break;
                }
            }
        } else if (FieldArea_AreEffectsActive((FieldAreaContext *)field_context, arguments[0])) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_PAUSE_FIELD_PALETTE_ANIMATION:
        if (arguments[0] == -1) {
            int animation_slot;

            for (animation_slot = 0; animation_slot < 2;
                 animation_slot++) {
                FieldArea_SetEffectsPaused((FieldAreaContext *)field_context, animation_slot, 1);
            }
        } else {
            FieldArea_SetEffectsPaused((FieldAreaContext *)field_context, arguments[0], 1);
        }
        break;

    case FIELD_VM_RESUME_FIELD_PALETTE_ANIMATION:
        if (arguments[0] == -1) {
            int animation_slot;

            for (animation_slot = 0; animation_slot < 2;
                 animation_slot++) {
                FieldArea_SetEffectsPaused((FieldAreaContext *)field_context, animation_slot, 0);
            }
        } else {
            FieldArea_SetEffectsPaused((FieldAreaContext *)field_context, arguments[0], 0);
        }
        break;

    case FIELD_VM_REVERSE_FIELD_PALETTE_ANIMATION:
        if (arguments[0] == -1) {
            int animation_slot;

            for (animation_slot = 0; animation_slot < 2;
                 animation_slot++) {
                FieldArea_ReverseEffects((FieldAreaContext *)field_context, animation_slot);
            }
        } else {
            FieldArea_ReverseEffects((FieldAreaContext *)field_context, arguments[0]);
        }
        break;

    case FIELD_VM_START_PALETTE_BANK_CROSSFADE:
        FieldPaletteCrossfade_Start(
            (FieldAreaContext *)field_context, arguments[0],
            arguments[1], arguments[2],
            arguments[3]);
        break;

    case FIELD_VM_WAIT_PALETTE_BANK_CROSSFADE:
        if ((*(FieldAsyncOperationFlags **)(field_context +
                FIELD_VM_PALETTE_CROSSFADE_STATE_OFFSET))->active) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_START_PATTERN_SCREEN_WIPE:
        func_ov000_02074810(
            field_context, arguments[0], 0);
        if (FieldVm_GetSpecialPartyState(field_context)->field_screen == 0 &&
            (arguments[0] == 0 ||
             arguments[0] == 2)) {
            FieldArea_ApplyQueuedMusic((FieldAreaContext *)field_context);
        }
        break;

    case FIELD_VM_START_CIRCULAR_SCREEN_WIPE: {
        circular_wipe_parameters.duration = arguments[0];
        circular_wipe_parameters.center_x = (u8)arguments[1];
        circular_wipe_parameters.center_y = (u8)arguments[2];
        circular_wipe_parameters.start_radius = (s16)arguments[3];
        circular_wipe_parameters.end_radius = (s16)arguments[4];
        circular_wipe_parameters.retain_after_completion = arguments[5];
        func_ov000_02074810(
            field_context, 4, &circular_wipe_parameters);
        if (FieldVm_GetSpecialPartyState(field_context)->field_screen == 0 &&
            arguments[3] == 0) {
            FieldArea_ApplyQueuedMusic((FieldAreaContext *)field_context);
        }
        break;
    }

    case FIELD_VM_START_RECTANGULAR_SCREEN_WIPE: {
        rectangular_wipe_parameters.duration = arguments[0];
        rectangular_wipe_parameters.center_x = (u8)arguments[1];
        rectangular_wipe_parameters.center_y = (u8)arguments[2];
        rectangular_wipe_parameters.start_left = (s16)arguments[3];
        rectangular_wipe_parameters.start_right = (s16)arguments[4];
        rectangular_wipe_parameters.start_top = (s16)arguments[5];
        rectangular_wipe_parameters.start_bottom = (s16)arguments[6];
        rectangular_wipe_parameters.end_left = (s16)arguments[7];
        rectangular_wipe_parameters.end_right = (s16)arguments[8];
        rectangular_wipe_parameters.end_top = (s16)arguments[9];
        rectangular_wipe_parameters.end_bottom = (s16)arguments[10];
        rectangular_wipe_parameters.retain_after_completion = arguments[11];
        func_ov000_02074810(
            field_context, 5, &rectangular_wipe_parameters);
        if (FieldVm_GetSpecialPartyState(field_context)->field_screen == 0 &&
            arguments[3] == 0 && arguments[4] == 0 &&
            arguments[5] == 0 && arguments[6] == 0) {
            FieldArea_ApplyQueuedMusic((FieldAreaContext *)field_context);
        }
        break;
    }

    case FIELD_VM_WAIT_SCREEN_WIPE:
        if ((*(FieldAsyncOperationFlags **)(field_context +
                FIELD_VM_SCREEN_WIPE_STATE_OFFSET))->active) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_SET_FIELD_SCREEN_ALPHA_BLEND: {
        int screen =
            FieldVm_GetSpecialPartyState(field_context)->field_screen;

        if (screen == 0) {
            func_02036988(
                (volatile u32 *)0x04000050, arguments[0], arguments[1],
                arguments[2], arguments[3]);
        } else {
            func_02036988(
                (volatile u32 *)0x04001050, arguments[0], arguments[1],
                arguments[2], arguments[3]);
        }
        break;
    }

    case FIELD_VM_START_TIME_HOLE_DEPARTURE:
        func_ov000_02069d38(
            *(void **)(field_context + FIELD_VM_FIELD_SYSTEM_OFFSET),
            arguments, arguments[4],
            arguments[5], arguments[6],
            arguments[7], arguments[8],
            arguments[9], arguments[10]);
        break;

    case FIELD_VM_PREPARE_TIME_HOLE_ARRIVAL:
        FieldSystem_PrepareTimeHoleArrival(
            *(FieldSystem **)(field_context + FIELD_VM_FIELD_SYSTEM_OFFSET),
            arguments, arguments[4],
            &arguments[5]);
        FieldArea_ApplyQueuedMusic((FieldAreaContext *)field_context);
        break;

    case FIELD_VM_START_PREPARED_TIME_HOLE_ARRIVAL:
        FieldSystem_StartPreparedTimeHoleArrival(
            *(FieldSystem **)(field_context + FIELD_VM_FIELD_SYSTEM_OFFSET));
        break;

    case FIELD_VM_START_TIME_HOLE_TUNNEL:
        func_ov000_02069284(
            field_system, arguments, arguments[4],
            arguments[5], 1, 0, arguments[6] != 0,
            (s16)arguments[7], (s16)arguments[8]);
        break;

    case FIELD_VM_START_FIELD_VERTICAL_ACCELERATING_SCROLL:
        FieldSystem_StartVerticalScroll(
            (FieldSystem *)field_system, arguments[6],
            FieldVm_DecodeWideArgument(command, 0),
            FieldVm_DecodeWideArgument(command, 2),
            FieldVm_DecodeWideArgument(command, 4));
        break;

    case FIELD_VM_DECELERATE_FIELD_VERTICAL_SCROLL:
        FieldSystem_DecelerateVerticalScroll(
            (FieldSystem *)field_system, -FieldVm_DecodeWideArgument(command, 0));
        break;

    case FIELD_VM_WAIT_FIELD_VERTICAL_SCROLL:
        if (FieldSystem_IsVerticalScrollActive((FieldSystem *)field_system)) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_BEGIN_TIME_HOLE_TUNNEL_WHITEOUT:
        *(u32 *)((u8 *)field_system + 0x274) |= 0x40000;
        break;

    case FIELD_VM_WAIT_TIME_HOLE_TRANSITION:
        if (FieldSystem_IsTimeHoleActive((FieldSystem *)field_system)) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_START_ANCHORED_FIELD_EFFECT_ANIMATION:
        FieldArea_StartModelAnimation(
            (FieldAreaContext *)field_context, runtime->screen_selector_bits.field_screen,
            arguments[0], arguments[1],
            arguments[2], arguments[3],
            arguments[4], (s16)arguments[5],
            (s16)arguments[6], (s16)arguments[7],
            arguments[8], (s16)arguments[9]);
        break;

    case FIELD_VM_WAIT_ANCHORED_FIELD_EFFECT_ANIMATION:
        if (FieldArea_IsModelAnimationActive((FieldAreaContext *)field_context)) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_START_FIELD_EFFECT_ANIMATION:
        FieldArea_StartSpriteAnimation(
            (FieldAreaContext *)field_context, arguments[0],
            runtime->screen_selector_bits.field_screen, arguments[1],
            (s16)arguments[2], (s16)arguments[3],
            (s16)arguments[4]);
        break;

    case FIELD_VM_WAIT_FIELD_EFFECT_ANIMATION:
        if (FieldArea_IsSpriteAnimationActive(
                (FieldAreaContext *)field_context, arguments[0])) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_RETURN_TO_TITLE_SCREEN:
        func_020052b0(16, 1);
        result = SCRIPT_VM_FINISHED;
        break;

    case FIELD_VM_LEGACY_NOOP_10C:
        break;

    case FIELD_VM_WAIT_PAIRED_FIELD_READY: {
        u8 *paired_context = *(u8 **)(
            field_context + FIELD_VM_PAIRED_FIELD_CONTEXT_OFFSET);

        if (paired_context == 0 ||
            *(u16 *)(paired_context + FIELD_VM_PAIRED_CONTEXT_TYPE_OFFSET) != 71) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;
    }

    case FIELD_VM_ENABLE_FIELD_TRIGGER_AREA: {
        u8 *trigger_areas = *(u8 **)(
            field_context + FIELD_VM_TRIGGER_AREAS_OFFSET);
        u32 *trigger_flags = (u32 *)(
            trigger_areas + 44 * arguments[0] + 40);

        *trigger_flags &= ~1;
        break;
    }

    case FIELD_VM_DISABLE_FIELD_TRIGGER_AREA: {
        u8 *trigger_areas = *(u8 **)(
            field_context + FIELD_VM_TRIGGER_AREAS_OFFSET);
        u32 *trigger_flags = (u32 *)(
            trigger_areas + 44 * arguments[0] + 40);

        *trigger_flags = (*trigger_flags & ~1) | 1;
        break;
    }

    case FIELD_VM_APPLY_SAVED_PARTY_CONFIGURATION: {
        FieldPartyManager_ApplyStoryPresence((FieldPartyManager *)party_manager, 0);
        if (VM_ReadVariable(0x2005, 0, 0) ||
            VM_ReadVariable(0x2006, 0, 0)) {
            *(u32 *)(field_context + FIELD_VM_PARTY_OWNERSHIP_MASKS_OFFSET) |=
                3;
            *(u32 *)(field_context + FIELD_VM_PARTY_OWNERSHIP_MASKS_OFFSET) &=
                ~0xC;
            *(u16 *)(field_context + FIELD_VM_SPECIAL_PARTY_STATE_OFFSET) =
                (*(u16 *)(field_context +
                          FIELD_VM_SPECIAL_PARTY_STATE_OFFSET) & ~0xC) |
                4;
        } else if (VM_ReadVariable(0x2007, 0, 0)) {
            *(u32 *)(field_context + FIELD_VM_PARTY_OWNERSHIP_MASKS_OFFSET) |=
                0xC;
            *(u32 *)(field_context + FIELD_VM_PARTY_OWNERSHIP_MASKS_OFFSET) &=
                ~3;
            *(u16 *)(field_context + FIELD_VM_SPECIAL_PARTY_STATE_OFFSET) =
                (*(u16 *)(field_context +
                          FIELD_VM_SPECIAL_PARTY_STATE_OFFSET) & ~0xC) |
                8;
        } else {
            *(u32 *)(field_context + FIELD_VM_PARTY_OWNERSHIP_MASKS_OFFSET) |=
                0xF;
            *(u16 *)(field_context + FIELD_VM_SPECIAL_PARTY_STATE_OFFSET) =
                (*(u16 *)(field_context +
                          FIELD_VM_SPECIAL_PARTY_STATE_OFFSET) & ~0xC) |
                0xC;
        }
        func_ov000_02079d74(field_context, 3);
        break;
    }

    case FIELD_VM_SET_FIELD_INPUT_DISABLE_MASK: {
        u8 *target_context = field_context;
        u16 previous_mask;

        if (arguments[0] == -1) {
            arguments[0] =
                FieldVm_GetSpecialPartyState(target_context)->field_screen;
        }
        if (arguments[0] !=
            FieldVm_GetSpecialPartyState(target_context)->field_screen) {
            target_context = *(u8 **)(
                target_context + FIELD_VM_PAIRED_FIELD_CONTEXT_OFFSET);
        }
        previous_mask = *(u16 *)(target_context +
                                 FIELD_VM_INPUT_DISABLE_MASK_OFFSET);
        *(u16 *)(target_context + FIELD_VM_INPUT_DISABLE_MASK_OFFSET) =
            arguments[1] ^ 0xFFFF;
        VM_WriteVariable(
            command->result_variable, previous_mask ^ 0xFFFF, vm,
            base_state);
        break;
    }

    case FIELD_VM_SET_FIELD_EVENT_INPUT_DISABLE_MASK: {
        u8 *target_context = field_context;
        u16 previous_mask;

        if (arguments[0] == -1) {
            arguments[0] =
                FieldVm_GetSpecialPartyState(target_context)->field_screen;
        }
        if (arguments[0] !=
            FieldVm_GetSpecialPartyState(target_context)->field_screen) {
            target_context = *(u8 **)(
                target_context + FIELD_VM_PAIRED_FIELD_CONTEXT_OFFSET);
        }
        previous_mask = *(u16 *)(
            target_context + FIELD_VM_EVENT_INPUT_DISABLE_MASK_OFFSET);
        *(u16 *)(target_context + FIELD_VM_EVENT_INPUT_DISABLE_MASK_OFFSET) =
            arguments[1] ^ 0xFFFF;
        VM_WriteVariable(
            command->result_variable, previous_mask ^ 0xFFFF, vm,
            base_state);
        break;
    }

    case FIELD_VM_SET_FIELD_INPUT_CHANNEL_OVERRIDE:
        *(u16 *)(field_context + FIELD_VM_INPUT_OVERRIDES_OFFSET +
                 2 * arguments[0]) = (u16)arguments[1];
        *(u16 *)(field_context +
                 FIELD_VM_INPUT_EXCLUSION_MASKS_OFFSET +
                 2 * arguments[0]) = arguments[2] ^ 0xFFFF;
        break;

    case FIELD_VM_RESET_FIELD_INPUT_CHANNEL_OVERRIDES: {
        *(u16 *)(field_context + FIELD_VM_INPUT_OVERRIDES_OFFSET + 0) = 0;
        *(u16 *)(field_context + FIELD_VM_INPUT_OVERRIDES_OFFSET + 2) = 0;
        *(u16 *)(field_context + FIELD_VM_INPUT_OVERRIDES_OFFSET + 4) = 0;
        *(u16 *)(field_context + FIELD_VM_INPUT_OVERRIDES_OFFSET + 6) = 0;
        *(u16 *)(field_context + FIELD_VM_INPUT_EXCLUSION_MASKS_OFFSET + 0) =
            0xFFFF;
        *(u16 *)(field_context + FIELD_VM_INPUT_EXCLUSION_MASKS_OFFSET + 2) =
            0xFFFF;
        *(u16 *)(field_context + FIELD_VM_INPUT_EXCLUSION_MASKS_OFFSET + 4) =
            0xFFFF;
        *(u16 *)(field_context + FIELD_VM_INPUT_EXCLUSION_MASKS_OFFSET + 6) =
            0xFFFF;
        break;
    }

    case FIELD_VM_SET_FIELD_INPUT_DIRECTION_ROTATION:
        *(u16 *)(field_context +
                 FIELD_VM_INPUT_DIRECTION_ROTATION_OFFSET) =
            (*(u16 *)(field_context +
                      FIELD_VM_INPUT_DIRECTION_ROTATION_OFFSET) & ~3) |
            ((u16)arguments[0] & 3);
        break;

    case FIELD_VM_SET_FIELD_PARTY_CONTROL_ENABLED:
        FieldVm_GetSpecialPartyState(field_context)->party_control_enabled =
            arguments[0];
        if (arguments[1] != -1) {
            int selected_party_side = (arguments[1] >> 1) & 1;
            int current_screen =
                FieldVm_GetSpecialPartyState(field_context)->field_screen;

            party_controller = FieldVm_GetPartyController(
                party_manager, selected_party_side);
            if (current_screen ==
                FieldVm_GetPartyControllerFlags(party_controller)
                    ->field_screen) {
                FieldVm_GetPartyControllerFlags(
                    FieldVm_GetPartyController(party_manager, 0))
                    ->active_member = arguments[1] & 1;
            }
        }
        break;

    case FIELD_VM_SET_CAMERA_FOCUS_ENTITY:
        FieldVm_GetSpecialPartyState(field_context)->camera_focus_enabled =
            arguments[0];
        if (arguments[1] != -1) {
            FieldVm_GetSpecialPartyState(field_context)
                ->camera_focus_entity_id = (s8)arguments[1];
            runtime_entity = FieldVm_GetRuntimeEntity(
                FieldVm_GetEntityByIndex(
                    field_context,
                    FieldVm_GetSpecialPartyState(field_context)
                        ->camera_focus_entity_id));
            *(fx32 *)(field_context +
                      FIELD_VM_CAMERA_FOCUS_X_OFFSET) =
                runtime_entity->position_x;
            *(fx32 *)(field_context +
                      FIELD_VM_CAMERA_FOCUS_Y_OFFSET) =
                runtime_entity->position_y - runtime_entity->position_z;
        }
        break;

    case FIELD_VM_REJOIN_PARTY_FOLLOWER:
        if ((FieldVm_GetPresentPartyMask(field_context) &
             (1 << arguments[0])) != 0) {
            FieldParty_RejoinFollower((FieldPartyController *)FieldVm_GetPartyController(
                party_manager, arguments[0]),
                arguments[1] != 0);
        }
        break;

    case FIELD_VM_WAIT_PARTY_FOLLOWER_REJOINED:
        if ((FieldVm_GetPresentPartyMask(field_context) &
             (1 << arguments[0])) != 0) {
            party_controller = FieldVm_GetPartyController(
                party_manager, arguments[0]);
            if (FieldVm_GetPartyControllerFlags(party_controller)
                    ->follower_rejoin_active != 0) {
                result = FieldVm_RetryCurrentCommand(
                    vm, state, command->opcode);
                break;
            }
        }
        break;

    case FIELD_VM_DETACH_PARTY_FOLLOWER:
        if ((FieldVm_GetPresentPartyMask(field_context) &
             (1 << arguments[0])) != 0) {
            FieldParty_DisableFollowing((FieldPartyController *)FieldVm_GetPartyController(
                party_manager, arguments[0]));
        }
        break;

    case FIELD_VM_REUNITE_SPLIT_PARTIES:
        if (arguments[0] == -1) {
            arguments[0] =
                FieldVm_GetActivePartySide(party_manager);
        }
        FieldPartyManager_BeginReunion(
            (FieldPartyManager *)party_manager, arguments[0],
            arguments[1] != 0);
        break;

    case FIELD_VM_WAIT_SPLIT_PARTY_REUNION:
        if (FieldPartyManager_IsReuniting((FieldPartyManager *)party_manager)) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_START_BABY_PIGGYBACK_MOUNT:
        FieldPartyManager_BeginPiggybackMount((FieldPartyManager *)party_manager);
        break;

    case FIELD_VM_WAIT_BABY_PIGGYBACK_MOUNT:
        if (FieldPartyManager_IsPiggybackMountActive((FieldPartyManager *)party_manager)) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_SET_PARTY_MEMBER_CHARACTER_ID:
        FieldParty_SetMemberAction(
            (FieldPartyController *)FieldVm_GetPartyController(
                party_manager, arguments[0] / 2),
            arguments[0] & 1, arguments[1]);
        break;

    case FIELD_VM_RESET_PARTY_MEMBER_CHARACTERS:
        FieldParty_ResetActionSelection((FieldPartyController *)FieldVm_GetPartyController(
            party_manager, arguments[0]));
        break;

    case FIELD_VM_SET_PARTY_INDICATOR_VISIBILITY:
        FieldParty_SetIndicatorVisibility(
            (FieldPartyController *)FieldVm_GetPartyController(
                party_manager, arguments[0]),
            arguments[1] != 0, arguments[2] != 0, 0);
        break;

    case FIELD_VM_SET_PARTY_MEMBER_SWITCHING_ENABLED:
        party_controller = party_manager +
            FIELD_VM_PARTY_CONTROLLER_OFFSET +
            FIELD_VM_PARTY_CONTROLLER_CONTROL_FLAGS_OFFSET +
            FIELD_VM_PARTY_CONTROLLER_SIZE * arguments[0];
        *(u32 *)party_controller =
            (*(u32 *)party_controller & ~(1 << 2)) |
            ((arguments[1] & 1) << 2);
        break;

    case FIELD_VM_LEGACY_NOOP_0B9:
        break;

    case FIELD_VM_SET_FIELD_PARTY_HUD_LAYOUT:
        if (FieldVm_GetSpecialPartyState(field_context)->field_screen == 0) {
            func_ov000_02071a38(
                *(u8 **)(field_context +
                         FIELD_VM_PAIRED_FIELD_CONTEXT_OFFSET),
                arguments[0] != 0,
                arguments[1] != 0);
        } else {
            func_ov000_02071a38(
                field_context, arguments[0] != 0,
                arguments[1] != 0);
        }
        break;

    case FIELD_VM_SET_PARTY_ATTACHED_SPRITE_VISIBLE:
        FieldParty_SetAttachedSpriteVisible(
            (FieldPartyController *)FieldVm_GetPartyController(party_manager, 1),
            arguments[0] != 0);
        break;

    case FIELD_VM_SET_PARTY_FIELD_MODE:
        if ((FieldVm_GetPresentPartyMask(field_context) &
             (1 << arguments[0])) != 0) {
            FieldPartyManager_SetFieldMode(
                (FieldPartyManager *)party_manager, arguments[0],
                (u16)arguments[1], 0, 0);
        }
        break;

    case FIELD_VM_CANCEL_PARTY_ACTIONS:
        if (arguments[0] == -1) {
            if ((FieldVm_GetPresentPartyMask(field_context) & 1) != 0) {
                FieldPartyManager_CancelActions(
                    (FieldPartyManager *)party_manager, 0, (u16)arguments[1]);
            }
            if ((FieldVm_GetPresentPartyMask(field_context) & 2) != 0) {
                FieldPartyManager_CancelActions(
                    (FieldPartyManager *)party_manager, 1, (u16)arguments[1]);
            }
        } else if ((FieldVm_GetPresentPartyMask(field_context) &
                    (1 << arguments[0])) != 0) {
            FieldPartyManager_CancelActions(
                (FieldPartyManager *)party_manager, arguments[0],
                (u16)arguments[1]);
        }
        break;

    case FIELD_VM_WAIT_PARTY_ACTIONS_IDLE:
        if ((FieldVm_GetPresentPartyMask(field_context) &
             (1 << arguments[0])) != 0 &&
            FieldPartyManager_HasActiveActions(
                (FieldPartyManager *)party_manager, arguments[0],
                FieldVm_GetPresentPartyMask(field_context))) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_START_BABY_HAMMER_SWING:
        FieldParty_BeginHammerSwing(
            (FieldPartyController *)FieldVm_GetPartyController(party_manager, 1),
            arguments[0] & 1);
        break;

    case FIELD_VM_WAIT_BABY_HAMMER_SWING:
        if (FieldParty_IsHammerActive(
                (FieldPartyController *)FieldVm_GetPartyController(party_manager, 1),
                arguments[0] & 1)) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_FINISH_BABY_HAMMER_SWING:
        FieldParty_EndHammerSwing(
            (FieldPartyController *)FieldVm_GetPartyController(party_manager, 1),
            arguments[0] & 1);
        break;

    case FIELD_VM_PREPARE_BABY_DRILL_MARKER:
        FieldPartyManager_PrepareBabyDrillMarker((FieldPartyManager *)party_manager, arguments[0]);
        break;

    case FIELD_VM_MOVE_PARTY_TO_GIMMICK_ANCHOR:
        FieldPartyManager_MoveToAnchor(
            (FieldPartyManager *)party_manager, arguments[0],
            arguments[1]);
        break;

    case FIELD_VM_PREPARE_BROS_BALL_CANNON_LAUNCH:
        FieldPartyManager_PrepareBrosBallLaunch((FieldPartyManager *)party_manager);
        break;

    case FIELD_VM_LAUNCH_BROS_BALL_FROM_CANNON:
        FieldPartyManager_LaunchBrosBall((FieldPartyManager *)party_manager, arguments[0]);
        break;

    case FIELD_VM_LAUNCH_BABY_PARTY_TO_ELEVATION:
        FieldPartyManager_LaunchBabiesToElevation(
            (FieldPartyManager *)party_manager,
            arguments[0] << FX32B_INT);
        break;

    case FIELD_VM_DROP_BABY_PARTY_FROM_ELEVATION:
        FieldPartyManager_DropBabies((FieldPartyManager *)party_manager);
        break;

    case FIELD_VM_COMPLETE_AIRBORNE_BABY_TRANSFERS:
        FieldPartyManager_CompleteAirborneTransfers((FieldPartyManager *)party_manager);
        break;

    case FIELD_VM_GET_PARTY_CONTROLLER_PROPERTY:
        VM_WriteVariable(
            command->result_variable,
            FieldVm_GetPartyControllerProperty(
                FieldVm_GetPartyController(
                    party_manager, arguments[0]),
                arguments[1]),
            vm, base_state);
        break;

    case FIELD_VM_SWITCH_ACTIVE_PARTY:
        if (arguments[0] == -1) {
            arguments[0] =
                FieldVm_GetActivePartySide(party_manager) ^ 1;
        }
        func_ov000_020a0c30(
            party_manager, arguments[0], arguments[1] != 0, 0, 1);
        break;

    case FIELD_VM_WAIT_ACTIVE_PARTY_SWITCH:
        if (FieldVm_GetPartyManagerFlags(party_manager)->
            active_party_switching) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_SET_PARTY_PIGGYBACK_STATE:
        *(u32 *)(party_manager + 4) =
            (*(u32 *)(party_manager + 4) & ~0x80) |
            ((arguments[0] & 1) << 7);
        break;

    case FIELD_VM_RELEASE_ROOM_TRANSITION_CONTROL_LOCK:
        *(u16 *)(field_context +
                 FIELD_VM_FIELD_CONTROL_FLAGS_OFFSET) &= ~8;
        break;

    case FIELD_VM_SET_PARTY_LEADER_ANIMATION_OVERRIDE:
        if (arguments[1] != 0) {
            FieldParty_StartLeaderPaletteAnimation((FieldPartyController *)FieldVm_GetPartyController(
                party_manager, arguments[0]), 0);
        } else {
            FieldParty_ClearLeaderPaletteAnimation((FieldPartyController *)FieldVm_GetPartyController(
                party_manager, arguments[0]), 0);
        }
        break;

    case FIELD_VM_REMOVE_ALL_ENTITY_EFFECT_SPRITES:
        FieldEffect_StopAll(field_context);
        break;

    case FIELD_VM_WAIT_ALL_ENTITY_EFFECT_SPRITES:
        if (FieldEffect_AnyActive(field_context)) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_LEGACY_NOOP_11A:
    case FIELD_VM_LEGACY_NOOP_11B:
        break;

    case FIELD_VM_START_SCRIPTED_BATTLE: {
        func_ov000_0206ba2c(
            field_system, 1, (s16)arguments[0],
            arguments[1],
            *(u16 *)(field_context +
                     FIELD_VM_CURRENT_ROOM_ID_OFFSET),
            arguments[3] & 0xFF,
            FieldVm_GetSpecialPartyState(field_context)->field_screen,
            FieldVm_GetEntityByIndex(field_context, arguments[4]),
            FieldVm_GetEntityByIndex(field_context, 0),
            arguments[5], 0);
        result = SCRIPT_VM_YIELDED;
        break;
    }

    case FIELD_VM_START_SCRIPTED_BATTLE_WITH_TRANSITION: {
        func_ov000_0206ba2c(
            field_system, 1, (s16)arguments[0],
            arguments[1],
            *(u16 *)(field_context +
                     FIELD_VM_CURRENT_ROOM_ID_OFFSET),
            arguments[3] & 0xFF,
            FieldVm_GetSpecialPartyState(field_context)->field_screen,
            FieldVm_GetEntityByIndex(field_context, arguments[4]),
            FieldVm_GetEntityByIndex(field_context, 0),
            arguments[5], arguments[6]);
        result = SCRIPT_VM_YIELDED;
        break;
    }

    case FIELD_VM_OPEN_PAUSE_MENU:
        FieldVm_ResolveMenuPartyContext(runtime, &arguments[0]);
        FieldSystem_RequestPause(
            field_system, 1, arguments[0], arguments[1],
            arguments[2] != 0, arguments[3] != 0);
        result = SCRIPT_VM_YIELDED;
        break;

    case FIELD_VM_SET_SAVE_LOCATION_ID:
        *(u16 *)((u8 *)field_system +
                 FIELD_VM_FIELD_SYSTEM_SAVE_LOCATION_OFFSET) =
            (u16)arguments[0];
        break;

    case FIELD_VM_OPEN_SAVE_MENU:
        FieldVm_ResolveMenuPartyContext(runtime, &arguments[0]);
        FieldSystem_RequestSave(
            field_system, arguments[0],
            arguments[1] != 0, arguments[2] != 0);
        result = SCRIPT_VM_YIELDED;
        break;

    case FIELD_VM_OPEN_SHOP:
        FieldSystem_RequestShop(
            field_system, arguments[0],
            arguments[1] != 0, arguments[2] != 0);
        result = SCRIPT_VM_YIELDED;
        break;

    case FIELD_VM_START_STAFF_CREDITS:
        FieldSystem_RequestCredits(field_system, arguments[0] != 0);
        result = SCRIPT_VM_YIELDED;
        break;

    case FIELD_VM_OPEN_GAME_OVER_MENU:
        FieldSystem_RequestGameOver(field_system, arguments[0] != 0);
        result = SCRIPT_VM_YIELDED;
        break;

    case FIELD_VM_INITIALIZE_NEW_GAME_PRESET:
        GameParty_Initialize(arguments[0]);
        break;

    case FIELD_VM_RESTORE_PARTY_MEMBER_HP:
        FieldSystem_RefillMemberHp((FieldSystem *)field_system, arguments[0]);
        break;

    case FIELD_VM_ADJUST_PARTY_MEMBER_HP:
        FieldSystem_AdjustMemberHp(
            (FieldSystem *)field_system, arguments[0], arguments[1]);
        break;

    case FIELD_VM_SET_COINS:
        *(u32 *)(gSaveData + 1160) = arguments[0];
        break;

    case FIELD_VM_ADJUST_COINS: {
        int updated_count = GameInventory_Add(
            0xFFFF, arguments[0]);

        VM_WriteVariable(
            command->result_variable,
            arguments[0] == updated_count, vm, base_state);
        break;
    }

    case FIELD_VM_ADJUST_BEANS: {
        int updated_count = GameInventory_Add(
            0xFFFE, arguments[0]);

        VM_WriteVariable(
            command->result_variable,
            arguments[0] == updated_count, vm, base_state);
        if (updated_count > 0) {
            VM_WriteVariable(0x201F, 1, 0, 0);
        }
        break;
    }

    case FIELD_VM_ADJUST_ITEM_COUNT: {
        int updated_count = GameInventory_Add(
            (u16)arguments[0], arguments[1]);

        VM_WriteVariable(
            command->result_variable,
            arguments[0] == updated_count, vm, base_state);
        break;
    }

    case FIELD_VM_GET_ROOM_COMPANION_ID:
        VM_WriteVariable(
            command->result_variable,
            data_02048f1a[2 * arguments[0]], vm, base_state);
        break;

    case FIELD_VM_ENABLE_TOUCHSCREEN_INPUT_TRACKING:
        ((FieldAsyncOperationFlags *)((u8 *)field_system +
            FIELD_VM_FIELD_SYSTEM_TOUCH_STATE_OFFSET))->active = 1;
        break;

    case FIELD_VM_DISABLE_TOUCHSCREEN_INPUT_TRACKING: {
        u32 *touch_state;

        touch_state = (u32 *)((u8 *)field_system +
                             FIELD_VM_FIELD_SYSTEM_TOUCH_STATE_OFFSET);
        *touch_state &= ~1;
        *touch_state &= 0xFFFFFFF1;
        *touch_state = (*touch_state & 0xFFFFE00F) | 0x1FF0;
        *touch_state = (*touch_state & 0xFFC01FFF) | 0x3FE000;
        break;
    }

    case FIELD_VM_SET_RETRY_CHECKPOINT:
        gSaveData[1302] = arguments[0];
        break;

    case FIELD_VM_SET_RETRY_PROGRESS:
        gSaveData[1303] = arguments[0];
        break;

    case FIELD_VM_CLEAR_EXTENDED_SAVE_FLAGS:
        FieldVm_FillExtendedSaveFlags(0);
        break;

    case FIELD_VM_SET_EXTENDED_SAVE_FLAGS:
        FieldVm_FillExtendedSaveFlags(1);
        break;

    case FIELD_VM_PREPARE_TOUCHSCREEN_MASK_ERASE:
        FieldSystem_PrepareMaskErase((FieldSystem *)field_system);
        break;

    case FIELD_VM_WAIT_TOUCHSCREEN_MASK_ERASE_READY:
        if (FieldSystem_IsTransferPreparing((FieldSystem *)field_system)) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_START_TOUCHSCREEN_MASK_ERASE:
        FieldSystem_ResumeTransfer((FieldSystem *)field_system);
        break;

    case FIELD_VM_WAIT_TOUCHSCREEN_MASK_ERASE_COMPLETE:
        if (FieldSystem_IsTransferActive((FieldSystem *)field_system)) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_PLAY_RUMBLE_PATTERN:
        GameRumble_PlayTimed(
            arguments[0] - 1, arguments[1]);
        break;

    case FIELD_VM_STOP_RUMBLE:
        if (data_0205a00c != 0) {
            GameRumble_Stop();
        }
        break;

    case FIELD_VM_SET_SINGLE_PASS_COLLISION:
        FieldVm_GetSpecialPartyState(field_context)->single_pass_collision =
            arguments[0];
        break;

    case FIELD_VM_SET_TIMER_VALUE: {
        void *timer;

        timer = *(void **)((u8 *)field_system +
                           FIELD_VM_FIELD_SYSTEM_TIMER_OFFSET);
        FieldTimer_SetValue(
            (FieldTimer *)timer, (s8)arguments[0],
            (s8)arguments[1], (s8)arguments[2]);
        break;
    }

    case FIELD_VM_SET_TIMER_DISPLAY_POSITION: {
        void *timer;

        if (arguments[0] == -1) {
            arguments[0] =
                FieldVm_GetSpecialPartyState(field_context)->field_screen;
        }
        if ((arguments[1] & 0xFFFF) == 0x8000) {
            arguments[1] = 87;
        }
        if ((arguments[2] & 0xFFFF) == 0x8000) {
            arguments[2] = 88;
        }
        timer = *(void **)((u8 *)field_system +
                           FIELD_VM_FIELD_SYSTEM_TIMER_OFFSET);
        FieldTimer_SetPosition(
            (FieldTimer *)timer, arguments[0], (s16)arguments[1],
            (s16)arguments[2]);
        break;
    }

    case FIELD_VM_SET_TIMER_VISIBLE: {
        void *timer;

        timer = *(void **)((u8 *)field_system +
                           FIELD_VM_FIELD_SYSTEM_TIMER_OFFSET);
        FieldTimer_SetVisible((FieldTimer *)timer, arguments[0] != 0);
        break;
    }

    case FIELD_VM_SET_TIMER_COUNT_RATE: {
        int count_enabled = arguments[1] != 0;
        u8 *timer;

        timer = *(u8 **)((u8 *)field_system +
                         FIELD_VM_FIELD_SYSTEM_TIMER_OFFSET);
        timer[1] = arguments[0];
        timer[0] = (timer[0] & ~8) | ((count_enabled & 1) << 3);
        break;
    }

    case FIELD_VM_PAUSE_TIMER: {
        u8 *timer;

        timer = *(u8 **)((u8 *)field_system +
                         FIELD_VM_FIELD_SYSTEM_TIMER_OFFSET);
        timer[0] |= 4;
        break;
    }

    case FIELD_VM_RESUME_TIMER: {
        u8 *timer;

        timer = *(u8 **)((u8 *)field_system +
                         FIELD_VM_FIELD_SYSTEM_TIMER_OFFSET);
        timer[0] &= ~4;
        break;
    }

    case FIELD_VM_STOP_TIMER: {
        u8 *timer;

        timer = *(u8 **)((u8 *)field_system +
                         FIELD_VM_FIELD_SYSTEM_TIMER_OFFSET);
        FieldTimer_SetVisible((FieldTimer *)timer, 0);
        timer[1] = 0;
        break;
    }

    case FIELD_VM_OPEN_SCREEN_MESSAGE: {
        int message_speed;
        int message_slot;

        FieldArea_ClearNotifications((FieldAreaContext *)field_context);
        message_speed = VM_ReadVariable(0xC000, vm, base_state);
        message_slot = func_ov000_02070930(
            field_context, arguments[4],
            arguments[0], arguments[1],
            arguments[2], arguments[3],
            arguments[5], arguments[6],
            arguments[7], 0, arguments[8],
            (arguments[9] ^ 1) != 0, arguments[11],
            arguments[12], arguments[10], message_speed,
            gSaveData + 68);
        VM_WriteVariable(
            command->result_variable, message_slot, vm, base_state);
        break;
    }

    case FIELD_VM_OPEN_ENTITY_MESSAGE: {
        FieldEntity *linked_owner;
        int message_speed;
        int message_slot;

        entity = FieldScript_ResolveEntity(
            runtime, state, arguments[0]);
        FieldArea_ClearNotifications((FieldAreaContext *)field_context);
        if (FieldVm_GetContextType(state) == FIELD_SCRIPT_OWNER_PRIMARY) {
            linked_owner = FieldVm_GetEntityByIndex(
                field_context, (s8)state->owner_data[0]);
        } else {
            linked_owner = 0;
        }
        message_speed = VM_ReadVariable(0xC000, vm, base_state);
        message_slot = func_ov000_020700ec(
            field_context, arguments[3], entity,
            arguments[1], arguments[2],
            arguments[4], arguments[5],
            arguments[6], arguments[7],
            (arguments[8] ^ 1) != 0, arguments[10],
            arguments[11], arguments[9], message_speed,
            gSaveData + 68, linked_owner);
        VM_WriteVariable(
            command->result_variable, message_slot, vm, base_state);
        break;
    }

    case FIELD_VM_WAIT_MESSAGE_FINISHED:
        if (FieldArea_HasOpenMessageWindow(
                (FieldAreaContext *)field_context, arguments[0]) &&
            FieldArea_HasNonClosingWindow(
                (FieldAreaContext *)field_context, arguments[0])) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_WAIT_MESSAGE_CLOSED:
        if (FieldArea_HasOpenMessageWindow(
                (FieldAreaContext *)field_context, arguments[0])) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_CLOSE_MESSAGE:
        FieldArea_CloseMessageWindows(
            (FieldAreaContext *)field_context, arguments[0]);
        break;

    case FIELD_VM_SET_MESSAGE_WINDOW_SLIDE_MASK_ENABLED:
        FieldArea_SetMessageWindowClipEnabled(
            (FieldAreaContext *)field_context, arguments[0],
            arguments[1] != 0);
        break;

    case FIELD_VM_START_MESSAGE_WINDOW_SLIDE_BY_SPEED:
        FieldArea_StartMessageWindowSlideBySpeed(
            (FieldAreaContext *)field_context, arguments[0],
            arguments[1] << FX32B_INT,
            arguments[2] << FX32B_INT, arguments[3]);
        break;

    case FIELD_VM_START_MESSAGE_WINDOW_SLIDE_OVER_FRAMES:
        func_ov000_0206f378(
            field_context, arguments[0],
            arguments[1] << FX32B_INT,
            arguments[2] << FX32B_INT, arguments[3]);
        break;

    case FIELD_VM_WAIT_MESSAGE_WINDOW_SLIDE:
        if (((FieldAsyncOperationFlags *)(field_context +
                FIELD_VM_MESSAGE_WINDOW_SLIDE_FLAGS_OFFSET))->active) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_PLAY_FIELD_SOUND: {
        int index;
        FieldPartyManagerSoundState *sound_state =
            (FieldPartyManagerSoundState *)party_manager;

        GameAudio_PlayEffectDelayed(
            (s16)arguments[0], arguments[1], -1);
        if (arguments[2] == 0) {
            break;
        }

        for (index = 0; index < 4; index++) {
            if (arguments[0] ==
                (sound_state->tracked_sounds[index] & 0x7FFF)) {
                sound_state->tracked_sounds[index] =
                    arguments[0] |
                    (FieldVm_GetSpecialPartyState(field_context)->
                         field_screen << 15);
                break;
            }
        }
        if (index == 4) {
            for (index = 0; index < 4; index++) {
                if (sound_state->tracked_sounds[index] == 0) {
                    sound_state->tracked_sounds[index] =
                        arguments[0] |
                        (FieldVm_GetSpecialPartyState(field_context)->
                             field_screen << 15);
                    break;
                }
            }
        }
        break;
    }

    case FIELD_VM_STOP_FIELD_SOUND: {
        int index;
        FieldPartyManagerSoundState *sound_state =
            (FieldPartyManagerSoundState *)party_manager;

        GameAudio_StopEffect((s16)arguments[0]);
        for (index = 0; index < 4; index++) {
            if (arguments[0] ==
                (sound_state->tracked_sounds[index] & 0x7FFF)) {
                sound_state->tracked_sounds[index] = 0;
                break;
            }
        }
        break;
    }

    case FIELD_VM_PLAY_BACKGROUND_MUSIC:
        GameAudio_PlayVoice((s16)arguments[0], -1);
        break;

    case FIELD_VM_STOP_BACKGROUND_MUSIC:
        GameAudio_StopVoice((s16)arguments[0]);
        break;

    case FIELD_VM_LOAD_BACKGROUND_MUSIC_RESOURCE: {
        u32 loaded_resources = GameAudio_GetMusicBanks();
        u16 primary_resource = (u16)loaded_resources;
        u16 secondary_resource = (u16)(loaded_resources >> 16);

        if ((arguments[1] == 0 && arguments[0] != primary_resource) ||
            (arguments[1] == 1 && arguments[0] != secondary_resource)) {
            GameAudio_LoadMusic(
                arguments[0], arguments[1] & 0xFF, 0xC00);
        }
        if (arguments[1] == 1) {
            *(u32 *)(party_manager + 4) &= ~0x80000000;
        }
        break;
    }

    case FIELD_VM_WAIT_BACKGROUND_MUSIC_RESOURCE:
        if (GameAudio_IsLoading()) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_ACTIVATE_BACKGROUND_MUSIC:
        if (((FieldMusicActivationFlags *)&gSaveData[1300])->disabled == 0) {
            u32 *party_flags;

            GameAudio_GetMusicBanks();
            GameAudio_SetMusic(arguments[0]);
            party_flags = (u32 *)(party_manager + 4);
            *party_flags = (*party_flags & 0xFF01FFFF) |
                ((arguments[0] & 0x7F) << 17);
            *party_flags = (*party_flags & 0x80FFFFFF) | 0x7F000000;
        }
        break;

    case FIELD_VM_FADE_OUT_BACKGROUND_MUSIC:
        GameAudio_StopMusic(arguments[0]);
        *(u32 *)(party_manager + 4) &= 0xFF01FFFF;
        break;

    case FIELD_VM_SET_GLOBAL_SOUND_MUTED:
        GameAudio_FadeMusic(arguments[0], -1);
        break;

    case FIELD_VM_LOAD_SOUND_GROUP_ASYNC:
        GameAudio_StartStream(arguments[0]);
        break;

    case FIELD_VM_WAIT_SOUND_GROUP_LOAD:
        if (GameAudio_IsStreamPlaying() == 1) {
            result = FieldVm_RetryCurrentCommand(
                vm, state, command->opcode);
            break;
        }
        break;

    case FIELD_VM_RELEASE_SOUND_GROUP:
        GameAudio_StopStream();
        break;
    }

    return result;
}
