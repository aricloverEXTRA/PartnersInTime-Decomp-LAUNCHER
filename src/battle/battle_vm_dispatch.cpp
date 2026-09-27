extern "C" {

/*
 * Battle script dispatcher (overlay 2, 0x02079950-0x0207E430).
 *
 * The battle language's opcode table: actor motion and animation, damage and
 * status application, targeting, effects, sound, inventory and the flow control
 * that drives a turn. Kept as one switch for the same reason as the field
 * dispatcher - the layout is part of the match.
 *
 * Commands that both this and the common-battle VM understand live in
 * battle_vm_common_dispatch.c; an opcode's operand contract is not transferable
 * between the two, so check docs/research/SCRIPT_VM_SEMANTICS.md for the
 * instance you are reading.
 */

#include <game/battle_effect_controls.h>
#include <game/battle_link_effect.h>
#include <game/battle_object_link.h>
#include <game/battle_sprite_grid_capture.h>
#include <game/battle_actor.h>
#include <game/battle_ai.h>
#include <game/battle_context.h>
#include <game/battle_damage.h>
#include <game/battle_display_capture.h>
#include <game/battle_effect.h>
#include <game/battle_enemy_data.h>
#include <game/battle_global_properties.h>
#include <game/battle_hit.h>
#include <game/battle_impact_effect.h>
#include <game/battle_mesh.h>
#include <game/battle_model_animation.h>
#include <game/battle_object.h>
#include <game/battle_particle_sweep.h>
#include <game/battle_raster_effects.h>
#include <game/battle_scene.h>
#include <game/battle_curve_motion.h>
#include <game/battle_script_properties.h>
#include <game/battle_status.h>
#include <game/battle_vm_motion.h>
#include <game/item.h>
#include <game/save_data.h>
#include <nitro/fx.h>
}
#include "battle_vm_operands.h"

/*
 * Complete high-level reconstruction of the battle-specific command range.
 * Opcodes outside 0x033..0x0E8 are delegated to the common battle VM handler,
 * exactly like the original dispatcher. The complete 19,168-byte function
 * matches the European ROM and is linked into the matching build.
 */

extern "C" {
extern int BattleMap_GetEncounterResourceIndex(int encounter_id);
extern s32 _s32_div_f(s32 numerator, s32 denominator);
extern int func_ov002_020be3e8(BattleSceneObject *object, int channel_index,
                               int direction_x, int direction_y,
                               int direction_z, int distance,
                               int initial_velocity, int terminal_velocity,
                               int prefer_longer_duration);
extern void func_ov002_020724b0(s16 *parameters, const void *keyframes,
                                u16 keyframe_count, int extent_q16,
                                int step_fixed);
extern void func_ov002_02077e78(BattleActor *actor, s16 animation_id,
                                int damage, int effect_offset_x,
                                int effect_offset_y);
extern int func_ov002_02079090(ScriptVm *vm, ScriptVmState *state,
                               ScriptVmCommand *command, int spawn_mode);
extern int func_ov002_02078e38(ScriptVm *vm, ScriptVmState *state,
                               ScriptVmCommand *command, int spawn_mode);
extern int func_ov002_02078bb4(ScriptVm *vm, ScriptVmState *state,
                               ScriptVmCommand *command, int spawn_mode);
extern void func_ov002_020bccc4(u16 source_object_id, u16 target_object_id,
                                u16 relaxation_period, u16 link_length,
                                s8 flipped_y_adjustment, s8 source_x_offset,
                                s8 source_y_offset, s8 target_x_offset,
                                s8 target_y_offset, s8 render_mode,
                                u16 color, int enabled);

extern int func_ov002_020ae9c0(int center_x, int center_y, int radius,
                               int band_width, int phase, int cutoff);
extern void func_ov002_020bba60(BattleSceneObject *object);
extern void func_ov002_020bb00c(BattleSceneObject *primary,
                                BattleSceneObject *secondary,
                                int solver_parameter, int link_parameter,
                                int target_x, int target_y, int vertical_bias);
extern int func_ov002_020badd8(BattleSceneObject *object,
                               int remaining_segment_offset);
extern int GameInventory_Add(u16 item_id, int count_delta);
extern int GameInventory_GetCount(u16 item_id);
extern int BattleItemList_RebuildActionItems(void);
extern int BattleItemList_RebuildUsableItems(void);
extern void BattleSound_Stop(int sound_task_id);
extern u8 data_02050290[];
extern u8 data_020505c4[];
}

enum BattleVmOpcode {
    BATTLE_VM_ALLOCATE_OBJECT_DATA_BUFFER = 0x033,
    BATTLE_VM_CONFIGURE_OBJECT_DATA_LOAD = 0x034,
    BATTLE_VM_ENSURE_OBJECT_DATA_LOADED = 0x035,
    BATTLE_VM_QUEUE_CAPTURE_SURFACE_UPLOAD = 0x036,
    BATTLE_VM_QUEUE_OBJECT_DATA_LOAD = 0x037,
    BATTLE_VM_QUEUE_OBJECT_DATA_ALTERNATE_LOAD = 0x038,
    BATTLE_VM_WAIT_OBJECT_DATA_LOAD = 0x039,
    BATTLE_VM_GET_OBJECT_DATA_RESOURCE = 0x03A,
    BATTLE_VM_BIND_ENTITY_RESOURCE = 0x03B,
    BATTLE_VM_RESET_OBJECT_DATA = 0x03C,
    BATTLE_VM_SET_OBJECT_ANIMATION = 0x03D,
    BATTLE_VM_START_SCRIPT_BY_ID = 0x03E,
    BATTLE_VM_CONTROL_SCRIPT_EXECUTION_LEGACY = 0x03F,
    BATTLE_VM_ASSIGN_SCRIPT_OWNER_GROUP = 0x040,
    BATTLE_VM_SWAP_OBJECT_SLOTS = 0x041,
    BATTLE_VM_SET_RUNTIME_FLAG_03 = 0x042,
    BATTLE_VM_SET_STATE_OVERRIDE = 0x043,
    BATTLE_VM_SET_RESULT_CODE = 0x044,
    BATTLE_VM_CLEAR_NEXT_STATE = 0x045,
    BATTLE_VM_GET_GLOBAL_PROPERTY = 0x046,
    BATTLE_VM_SET_GLOBAL_PROPERTY = 0x047,
    BATTLE_VM_FIND_MOST_DAMAGED_ENEMY = 0x048,
    BATTLE_VM_LEGACY_NOOP_049 = 0x049,
    BATTLE_VM_GET_ENCOUNTER_ID = 0x04A,
    BATTLE_VM_GET_ENCOUNTER_RESOURCE_INDEX = 0x04B,
    BATTLE_VM_LEGACY_NOOP_04C = 0x04C,
    BATTLE_VM_GET_ACTOR_PROPERTY = 0x04D,
    BATTLE_VM_SET_ACTOR_PROPERTY = 0x04E,
    BATTLE_VM_SET_ACTOR_PROPERTY_FIXED = 0x04F,
    BATTLE_VM_ALIGN_OBJECT_VIEW_BASELINE = 0x050,
    BATTLE_VM_LEGACY_NOOP_051 = 0x051,
    BATTLE_VM_APPLY_OBJECT_MOVEMENT = 0x052,
    BATTLE_VM_GET_OBJECT_VIEW_COORDINATE = 0x053,
    BATTLE_VM_GET_ENEMY_STAT = 0x054,
    BATTLE_VM_SET_ENEMY_STAT = 0x055,
    BATTLE_VM_WAIT_OBJECT_PENDING_STATE = 0x056,
    BATTLE_VM_SET_ACTIVE_MODEL_ANIMATION = 0x057,
    BATTLE_VM_CONFIGURE_ANIMATION_LAYER = 0x058,
    BATTLE_VM_SET_ANIMATION_LAYER_STATE = 0x059,
    BATTLE_VM_INVOKE_MODEL_RELATION_TEST = 0x05A,
    BATTLE_VM_MOVE_OBJECT = 0x05B,
    BATTLE_VM_START_SINUSOIDAL_DIRECTION_MOTION = 0x05C,
    BATTLE_VM_MOVE_OBJECT_AT_SPEED = 0x05D,
    BATTLE_VM_START_AXIS_KINEMATIC_MOTION = 0x05E,
    BATTLE_VM_MOVE_WITH_KINEMATIC_DURATION = 0x05F,
    BATTLE_VM_START_AXIS_KINEMATIC_MOTION_GET_DURATION = 0x060,
    BATTLE_VM_MOVE_WITH_KINEMATIC_DURATION_GET_DURATION = 0x061,
    BATTLE_VM_START_DIRECTIONAL_KINEMATIC_A = 0x062,
    BATTLE_VM_START_DIRECTIONAL_KINEMATIC_B = 0x063,
    BATTLE_VM_START_DIRECTIONAL_BALLISTIC = 0x064,
    BATTLE_VM_START_DIRECTIONAL_KINEMATIC_A_GET_DURATION = 0x065,
    BATTLE_VM_START_DIRECTIONAL_KINEMATIC_B_GET_DURATION = 0x066,
    BATTLE_VM_START_DIRECTIONAL_BALLISTIC_GET_DURATION = 0x067,
    BATTLE_VM_START_VELOCITY_MOTION = 0x068,
    BATTLE_VM_START_ACCELERATION_MOTION = 0x069,
    BATTLE_VM_START_ACCELERATED_MOTION = 0x06A,
    BATTLE_VM_START_ACCELERATED_MOTION_WITH_RATIO = 0x06B,
    BATTLE_VM_START_ACCELERATED_MOTION_BETWEEN_VELOCITIES = 0x06C,
    BATTLE_VM_ROTATE_OBJECT_AROUND_AXIS = 0x06D,
    BATTLE_VM_WAIT_ANIMATION = 0x06E,
    BATTLE_VM_STOP_MOTION = 0x06F,
    BATTLE_VM_SNAPSHOT_POSITION = 0x070,
    BATTLE_VM_START_KEYFRAMED_MOTION = 0x071,
    BATTLE_VM_GET_MOTION_FRAME = 0x072,
    BATTLE_VM_START_AUX_SCRIPT = 0x073,
    BATTLE_VM_WAIT_AUX_SCRIPT = 0x074,
    BATTLE_VM_SUPPRESS_ACTOR_RENDERING = 0x075,
    BATTLE_VM_RESTORE_ACTOR_RENDERING = 0x076,
    BATTLE_VM_LEGACY_NOOP_077 = 0x077,
    BATTLE_VM_REMOVE_ENEMY = 0x078,
    BATTLE_VM_LEGACY_NOOP_079 = 0x079,
    BATTLE_VM_LEGACY_NOOP_07A = 0x07A,
    BATTLE_VM_LEGACY_NOOP_07B = 0x07B,
    BATTLE_VM_LEGACY_NOOP_07C = 0x07C,
    BATTLE_VM_DAMAGE_ACTOR = 0x07D,
    BATTLE_VM_HEAL_ACTOR = 0x07E,
    BATTLE_VM_WAIT_HIT_QUEUE = 0x07F,
    BATTLE_VM_CALCULATE_EQUIPMENT_DAMAGE = 0x080,
    BATTLE_VM_APPLY_STATUS = 0x081,
    BATTLE_VM_CLEAR_STATUS = 0x082,
    BATTLE_VM_SET_ACTOR_TARGETING_ENABLED = 0x083,
    BATTLE_VM_CONFIGURE_HIT = 0x084,
    BATTLE_VM_REFLECT_QUEUED_HITS = 0x085,
    BATTLE_VM_FIND_HIT_DESCRIPTOR = 0x086,
    BATTLE_VM_GET_QUEUED_HIT_X = 0x087,
    BATTLE_VM_GET_QUEUED_HIT_Y = 0x088,
    BATTLE_VM_GET_QUEUED_HIT_Z = 0x089,
    BATTLE_VM_GET_QUEUED_HIT_KIND = 0x08A,
    BATTLE_VM_DAMAGE_PARTY = 0x08B,
    BATTLE_VM_SET_AUXILIARY_OBJECTS_PAUSED = 0x08C,
    BATTLE_VM_CALCULATE_DAMAGE = 0x08D,
    BATTLE_VM_SPAWN_MODEL_EFFECT = 0x08E,
    BATTLE_VM_SPAWN_ATTACHED_MODEL_EFFECT = 0x08F,
    BATTLE_VM_SPAWN_MODEL_EFFECT_HANDLE = 0x090,
    BATTLE_VM_SPAWN_ALTERNATE_MODEL_EFFECT_HANDLE = 0x091,
    BATTLE_VM_SPAWN_SPRITE_EFFECT = 0x092,
    BATTLE_VM_SPAWN_ATTACHED_SPRITE_EFFECT = 0x093,
    BATTLE_VM_SPAWN_SPRITE_EFFECT_HANDLE = 0x094,
    BATTLE_VM_SPAWN_ATTACHED_SPRITE_EFFECT_HANDLE = 0x095,
    BATTLE_VM_SET_ATTACHED_EFFECT_USER_VALUE = 0x096,
    BATTLE_VM_SPAWN_MODEL_EFFECT_AT_WORLD_POSITION = 0x097,
    BATTLE_VM_SPAWN_MODEL_EFFECT_AT_OBJECT_POSITION = 0x098,
    BATTLE_VM_SPAWN_MODEL_EFFECT_AT_WORLD_POSITION_HANDLE = 0x099,
    BATTLE_VM_SPAWN_MODEL_EFFECT_AT_OBJECT_POSITION_HANDLE = 0x09A,
    BATTLE_VM_LEGACY_NOOP_09B = 0x09B,
    BATTLE_VM_LEGACY_NOOP_09C = 0x09C,
    BATTLE_VM_SET_TAGGED_EFFECT_FLAG = 0x09D,
    BATTLE_VM_CLEAR_TAGGED_EFFECT_STATE = 0x09E,
    BATTLE_VM_WAIT_SCRIPT_HANDLE = 0x09F,
    BATTLE_VM_IS_SCRIPT_HANDLE_ACTIVE = 0x0A0,
    BATTLE_VM_STRETCH_OBJECT_BETWEEN_ANCHORS = 0x0A1,
    BATTLE_VM_START_IMPACT_TRAIL_EMITTER = 0x0A2,
    BATTLE_VM_STOP_IMPACT_TRAIL_EMITTER = 0x0A3,
    BATTLE_VM_START_INTERPOLATED_OBJECT_LINK = 0x0A4,
    BATTLE_VM_STOP_INTERPOLATED_OBJECT_LINK = 0x0A5,
    BATTLE_VM_CANCEL_INTERPOLATED_OBJECT_LINK = 0x0A6,
    BATTLE_VM_START_SCREEN_PARTICLE_SWEEP = 0x0A7,
    BATTLE_VM_START_SCREEN_PARTICLE_SWEEP_HANDLE = 0x0A8,
    BATTLE_VM_START_GROUND_RIPPLE = 0x0A9,
    BATTLE_VM_START_GROUND_RIPPLE_HANDLE = 0x0AA,
    BATTLE_VM_DRAW_PROCEDURAL_GROUND_RIPPLES = 0x0AB,
    BATTLE_VM_SCATTER_MODEL_PARTS = 0x0AC,
    BATTLE_VM_START_MODEL_CRUSH = 0x0AD,
    BATTLE_VM_GET_MODEL_PART_EFFECT_STATE = 0x0AE,
    BATTLE_VM_TRANSFORM_MODEL_EFFECT_BASIS = 0x0AF,
    BATTLE_VM_START_FLEXIBLE_OBJECT_LINK = 0x0B0,
    BATTLE_VM_GET_FLEXIBLE_LINK_HEADING = 0x0B1,
    BATTLE_VM_START_SEGMENTED_OBJECT_LINK = 0x0B2,
    BATTLE_VM_INITIALIZE_SPRITE_GRID_CAPTURE = 0x0B3,
    BATTLE_VM_START_SPRITE_GRID_CAPTURE = 0x0B4,
    BATTLE_VM_ADVANCE_SPRITE_GRID_CAPTURE = 0x0B5,
    BATTLE_VM_FINISH_SPRITE_GRID_CAPTURE = 0x0B6,
    BATTLE_VM_IS_SPRITE_GRID_CAPTURE_ACTIVE = 0x0B7,
    BATTLE_VM_SPAWN_OBJECT_DATA_EFFECT_AT_WORLD_POSITION = 0x0B8,
    BATTLE_VM_SPAWN_OBJECT_DATA_EFFECT_AT_OBJECT_POSITION = 0x0B9,
    BATTLE_VM_SPAWN_OBJECT_DATA_EFFECT_AT_WORLD_POSITION_HANDLE = 0x0BA,
    BATTLE_VM_SPAWN_OBJECT_DATA_EFFECT_AT_OBJECT_POSITION_HANDLE = 0x0BB,
    BATTLE_VM_CONFIGURE_GLOBAL_RESOURCE_SLOTS = 0x0BC,
    BATTLE_VM_SPAWN_ARCHIVE_EFFECT_AT_WORLD_POSITION = 0x0BD,
    BATTLE_VM_SPAWN_ARCHIVE_EFFECT_AT_OBJECT_POSITION = 0x0BE,
    BATTLE_VM_SPAWN_ARCHIVE_EFFECT_AT_WORLD_POSITION_HANDLE = 0x0BF,
    BATTLE_VM_SPAWN_ARCHIVE_EFFECT_AT_OBJECT_POSITION_HANDLE = 0x0C0,
    BATTLE_VM_START_RASTER_RESOURCE_TRANSITION = 0x0C1,
    BATTLE_VM_START_FINITE_RASTER_EFFECT = 0x0C2,
    BATTLE_VM_LEGACY_NOOP_0C3 = 0x0C3,
    BATTLE_VM_YIELD_ONE_FRAME = 0x0C4,
    BATTLE_VM_SET_SUB_SCREEN_BLEND_COEFFICIENTS = 0x0C5,
    BATTLE_VM_SET_SUB_SCREEN_BLEND_PLANES = 0x0C6,
    BATTLE_VM_ADJUST_INVENTORY = 0x0C7,
    BATTLE_VM_GET_INVENTORY_COUNT = 0x0C8,
    BATTLE_VM_GET_ITEM_BATTLE_ANIMATION = 0x0C9,
    BATTLE_VM_LEGACY_NOOP_0CA = 0x0CA,
    BATTLE_VM_CONFIGURE_CONTROL_STATE = 0x0CB,
    BATTLE_VM_UPDATE_CONTROL_MASK = 0x0CC,
    BATTLE_VM_SET_INPUT_DISABLE_MASK = 0x0CD,
    BATTLE_VM_SET_INPUT_DIRECTION_ROTATION = 0x0CE,
    BATTLE_VM_LEGACY_NOOP_0CF = 0x0CF,
    BATTLE_VM_LEGACY_NOOP_0D0 = 0x0D0,
    BATTLE_VM_START_INLINE_OBJECT_SCRIPT = 0x0D1,
    BATTLE_VM_START_INLINE_OBJECT_SCRIPT_AND_YIELD = 0x0D2,
    BATTLE_VM_WAIT_INLINE_OBJECT_SCRIPT_AND_SKIP = 0x0D3,
    BATTLE_VM_START_RELATIVE_OBJECT_SCRIPT = 0x0D4,
    BATTLE_VM_WAIT_OBJECT_SCRIPT = 0x0D5,
    BATTLE_VM_WAIT_OBJECT_SCRIPTS_BY_OWNER = 0x0D6,
    BATTLE_VM_STOP_OBJECT_SCRIPT = 0x0D7,
    BATTLE_VM_STOP_OBJECT_SCRIPTS_BY_OWNER = 0x0D8,
    BATTLE_VM_PAUSE_OBJECT_SCRIPT = 0x0D9,
    BATTLE_VM_PAUSE_OBJECT_SCRIPTS_BY_OWNER = 0x0DA,
    BATTLE_VM_RESUME_OBJECT_SCRIPT = 0x0DB,
    BATTLE_VM_RESUME_OBJECT_SCRIPTS_BY_OWNER = 0x0DC,
    BATTLE_VM_BRANCH_COMPARE = 0x0DD,
    BATTLE_VM_BRANCH_OBJECT_PROPERTY = 0x0DE,
    BATTLE_VM_BRANCH_ACTOR_HIT_LOCKED = 0x0DF,
    BATTLE_VM_LEGACY_NOOP_0E0 = 0x0E0,
    BATTLE_VM_BRANCH_SCRIPT_ACTIVE = 0x0E1,
    BATTLE_VM_BRANCH_ANIMATION_ACTIVE = 0x0E2,
    BATTLE_VM_PLAY_SOUND_WITH_HANDLE = 0x0E3,
    BATTLE_VM_PLAY_SOUND = 0x0E4,
    BATTLE_VM_STOP_SOUND_TASK = 0x0E5,
    BATTLE_VM_STOP_ALL_SOUND_TASKS = 0x0E6,
    BATTLE_VM_WAIT_SOUND_TASK = 0x0E7,
    BATTLE_VM_SPAWN_RASTER_PARTICLE = 0x0E8
};

enum BattleVmHandleTag {
    BATTLE_VM_HANDLE_INDEX_MASK = 0x3FFF,
    BATTLE_VM_MODEL_EFFECT_HANDLE_TAG = 0x4000,
    BATTLE_VM_TASK_HANDLE_TAG = 0x4000,
    BATTLE_VM_SPRITE_EFFECT_HANDLE_TAG = 0x8000,
    BATTLE_VM_ATTACHED_EFFECT_HANDLE_TAG = 0xC000,
    BATTLE_VM_HANDLE_TAG_MASK = 0xC000
};

enum BattleVmScriptControlMode {
    BATTLE_VM_SCRIPT_STOP = 1,
    BATTLE_VM_SCRIPT_RESUME = 2,
    BATTLE_VM_SCRIPT_PAUSE = 3
};

enum BattleVmContextOffset {
    BATTLE_VM_NEXT_STATE_OFFSET = 0x18,
    BATTLE_VM_STATE_OVERRIDE_OFFSET = 0x1C,
    BATTLE_VM_STATUS_DURATION_OFFSET = 0x12C,
    BATTLE_VM_HIT_QUEUE_OFFSET = 0xCAD8,
    BATTLE_VM_EFFECT_VIEW_X_OFFSET = 0xCB9C,
    BATTLE_VM_EFFECT_VIEW_Y_OFFSET = 0xCB9E,
    BATTLE_VM_SPRITE_EFFECT_SLOTS_OFFSET = 0xCBF8,
    BATTLE_VM_ATTACHED_EFFECT_SLOTS_OFFSET = 0xCCF8,
    BATTLE_VM_TASK_HANDLE_SLOTS_OFFSET = 0xCDF8,
    BATTLE_VM_OBJECT_SCRIPT_STATES_OFFSET = 0x6D44,
    BATTLE_VM_SUB_SCREEN_BLEND_EVB_OFFSET = 0x30,
    BATTLE_VM_SUB_SCREEN_BLEND_EVA_OFFSET = 0x32,
    BATTLE_VM_SUB_SCREEN_BLEND_SECOND_PLANES_OFFSET = 0x34,
    BATTLE_VM_SUB_SCREEN_BLEND_FIRST_PLANES_OFFSET = 0x36,
    BATTLE_VM_INPUT_DIRECTION_ROTATION_OFFSET = 0x108,
    BATTLE_VM_INPUT_DISABLE_MASK_OFFSET = 0x10A,
    BATTLE_VM_CONTROL_STATE_OFFSET = 0x10E,
    BATTLE_VM_SOUND_TASK_SLOTS_OFFSET = 0xC8B4
};

enum BattleVmActorLayout {
    BATTLE_VM_ENEMY_ACTOR_TABLE_OFFSET = 0x6508,
    BATTLE_VM_ENEMY_ACTOR_FIRST_ID = 60,
    BATTLE_VM_ACTOR_ACTION_STATE_OFFSET = 0x70,
    BATTLE_VM_ACTOR_REACTION_STATE_OFFSET = 0x128,
    BATTLE_VM_ACTOR_AUXILIARY_STATE_OFFSET = 0x1E0
};

enum BattleVmLimit {
    BATTLE_VM_OBJECT_SCRIPT_STATE_COUNT = 40,
    BATTLE_VM_SOUND_TASK_COUNT = 16
};

enum BattleVmSaveOffset {
    BATTLE_VM_SAVE_ENCOUNTER_ID_OFFSET = 0x55C,
    BATTLE_VM_SAVE_RESULT_CODE_OFFSET = 0x55E
};

typedef struct BattleVmPosition {
    s16 x;
    s16 y;
    s16 z;
} BattleVmPosition;

static inline void BattleVm_WriteResult(ScriptVm *vm, ScriptVmState *state,
                                 ScriptVmCommand *command, s32 value) {
    VM_WriteVariable(command->result_variable, value, vm, state);
}

static inline int BattleVm_RetryCurrentCommand(
    ScriptVm *vm, ScriptVmState *state, u16 opcode
) {
    s32 descriptor = (u16)vm->command_descriptors[opcode];
    s32 halfword_count =
        ((descriptor & SCRIPT_VM_HAS_RESULT) >> 5) +
        (descriptor & SCRIPT_VM_ARGUMENT_COUNT_MASK) +
        ((descriptor & SCRIPT_VM_HAS_ARGUMENT_MODES) >> 6) + 1;

    state->script -= halfword_count;
    return BATTLE_AI_VM_YIELD;
}

static inline void BattleVm_StoreObjectViewPosition(
    BattlePosition *position, BattleSceneObject *object) {
    BattlePosition_StoreViewRelative(
        position, object->x, (s16)(object->y - object->z),
        (s16)(object->effect_anchor_z + 16 * (256 - object->y)),
        object->flags.bits.use_raw_position,
        object->flags.bits.use_alternate_model);
}

static inline void BattleVm_StorePackedObjectViewPosition(
    BattleVmPosition *position, BattleSceneObject *object) {
    BattleVm_StoreObjectViewPosition((BattlePosition *)position, object);
}

static inline void BattleVm_RefreshModelPalette(BattleModel *model) {
    const void *palette = model->get_palette_source();

    model->set_palette(palette);
}

static inline void BattleVm_SetActiveModelAnimation(BattleModel *model,
                                                    ScriptVmCommand *command) {
    int active_animation_id = model->get_animation_id();

    model->set_primary_animation(
        (u8)active_animation_id, (s16)command->arguments[1], 1);
}

static inline const u8 *BattleVm_GetActionItemRecord(u16 tagged_item_id) {
    if ((tagged_item_id & ITEM_ID_ACTION_TAG) != ITEM_ID_ACTION_TAG) {
        return 0;
    }
    return (const u8 *)&gActionItemRecords[
        tagged_item_id & ITEM_ID_INDEX_MASK];
}

static inline const u8 *BattleVm_GetUsableItemRecord(u16 tagged_item_id) {
    if ((tagged_item_id & ITEM_ID_USABLE_TAG) != ITEM_ID_USABLE_TAG) {
        return 0;
    }
    return (const u8 *)&gItemRecords[tagged_item_id & ITEM_ID_INDEX_MASK];
}

static inline const u8 *BattleVm_GetBadgeRecord(u16 tagged_item_id) {
    if ((tagged_item_id & ITEM_ID_BADGE_TAG) != ITEM_ID_BADGE_TAG) {
        return 0;
    }
    return &data_02050290[20 * (tagged_item_id & ITEM_ID_INDEX_MASK)];
}

static inline const u8 *BattleVm_GetClothingRecord(u16 tagged_item_id) {
    if ((tagged_item_id & ITEM_ID_CLOTHING_TAG) != ITEM_ID_CLOTHING_TAG) {
        return 0;
    }
    return &data_020505c4[28 * (tagged_item_id & ITEM_ID_INDEX_MASK)];
}

static inline const u8 *BattleVm_GetItemRecord(u16 tagged_item_id) {
    switch (tagged_item_id & ITEM_ID_TAG_MASK) {
    case ITEM_ID_ACTION_TAG:
        return BattleVm_GetActionItemRecord(tagged_item_id);
    case ITEM_ID_USABLE_TAG:
        return BattleVm_GetUsableItemRecord(tagged_item_id);
    case ITEM_ID_BADGE_TAG:
        return BattleVm_GetBadgeRecord(tagged_item_id);
    case ITEM_ID_CLOTHING_TAG:
        return BattleVm_GetClothingRecord(tagged_item_id);
    default:
        return 0;
    }
}

int BattleAI_DispatchOpcode(ScriptVm *vm, ScriptVmState *state,
                            ScriptVmCommand *command) {
    BattleAIState *ai_state = (BattleAIState *)state;
    BattleObjectDataLoadState *load_state;
    BattleSceneObject *object;
    BattleSceneObject *reference;
    BattleModel *model;
    BattleActor *actor;
    BattleEnemyActor *enemy;
    BattleEnemyStatRecord *enemy_stats;
    BattleAIState *aux_state;
    BattleMotionChannel *motion_channel;
    BattleHitDescriptor *hit_descriptor;
    BattleHitRecord *hit_record;
    BattlePartyActor *party_actor;
    BattleEffect *effect;
    BattleModelEffect *model_effect;
    BattleEffect **effect_slots;
    BattleAITask *task;
    BattleAIState *object_script_state;
    s16 *motion_parameters;
    const s32 *keyframe_record;
    const u16 *script;
    const u8 *item_record;
    BattlePosition alignment_position;
    BattleVmPosition coordinate_position;
    BattlePosition damage_position;
    BattleVmPosition party_damage_position;
    BattleVmPosition attached_model_position;
    BattleVmPosition model_handle_position;
    BattleVmPosition alternate_model_handle_position;
    BattleVmPosition attached_sprite_position;
    BattleVmPosition attached_sprite_handle_position;
    BattleModel *resource_sources[4];
    u16 object_id;
    int script_target_id;
    int mode;
    int x;
    int y;
    int z;
    int duration;
    int damage;
    int handle;
    int handle_index;
    int handle_tag;
    int effect_index;
    int slot_index;
    int script_selector;
    int comparison;
    int sound_task_id;
    int index;
    int actor_index;

    switch (command->opcode) {
    case BATTLE_VM_ALLOCATE_OBJECT_DATA_BUFFER:
        BattleObjectData_AllocateLoadBuffer(
            (u16)command->arguments[0],
            (u32)(u16)command->arguments[1] << 2);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_CONFIGURE_OBJECT_DATA_LOAD:
        BattleObjectData_ConfigureLoad(
            (u16)command->arguments[0], (u16)command->arguments[1],
            (u16)command->arguments[2]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_ENSURE_OBJECT_DATA_LOADED:
        BattleObjectData_EnsureLoaded(
            (u16)command->arguments[0],
            (s32)BattleVm_PackHalfwords(command->arguments[1],
                                        command->arguments[2]));
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_QUEUE_CAPTURE_SURFACE_UPLOAD:
        load_state = &((BattleObjectDataLoadState *)(
            gBattleContext + BATTLE_RUNTIME_OBJECT_DATA_LOAD_STATES_OFFSET))[
                (u16)command->arguments[0]];
        BattleObjectData_QueueCaptureSurfaceUpload(
            load_state, (u16)command->arguments[1],
            (u16)command->arguments[2]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_QUEUE_OBJECT_DATA_LOAD:
        func_ov002_02091fd8(
            (u16)command->arguments[0],
            (s32)BattleVm_PackHalfwords(command->arguments[1],
                                        command->arguments[2]));
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_QUEUE_OBJECT_DATA_ALTERNATE_LOAD:
        func_ov002_02091f68((u16)command->arguments[0],
                            command->arguments[1]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_WAIT_OBJECT_DATA_LOAD:
        if (BattleObjectData_IsLoadPending((u16)command->arguments[0])) {
            return BattleVm_RetryCurrentCommand(
                vm, state, BATTLE_VM_WAIT_OBJECT_DATA_LOAD
            );
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_GET_OBJECT_DATA_RESOURCE:
        load_state = BattleObjectData_GetLoadState(
            (u16)command->arguments[0]);
        BattleVm_WriteResult(vm, state, command, load_state->resource_id);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_BIND_ENTITY_RESOURCE:
        BattleEntity_BindResource((u16)command->arguments[0],
                                  (u16)command->arguments[1]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_RESET_OBJECT_DATA:
        BattleObjectData_ResetAll();
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SET_OBJECT_ANIMATION:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        model = BattleSceneObject_GetActiveModel(object);
        if (model != 0) {
            model->flags &= ~BATTLE_MODEL_FLAG_09;
        }
        BattleSceneObject_SetAnimation(
            BattleSceneObject_GetById((u16)command->arguments[0]),
            command->arguments[1], command->arguments[2]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_SCRIPT_BY_ID:
        BattleAI_StartScriptById((u16)command->arguments[0]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_CONTROL_SCRIPT_EXECUTION_LEGACY:
        /* The original descriptor exposes only the selector. The handler also
         * consumes the following decoded word as a legacy control mode. */
        if (command->arguments[1] == BATTLE_VM_SCRIPT_STOP &&
            (command->arguments[0] & BATTLE_AI_TASK_TYPE_MASK) >=
                BATTLE_AI_TASK_ACTION) {
            BattleAI_StopScriptById((u16)command->arguments[0]);
        } else {
            script_target_id = command->arguments[0] & 0xFFFF;
            switch ((u16)script_target_id) {
            case 1:
                aux_state = (BattleAIState *)(
                    gBattleContext + BATTLE_AI_PARTY_STATE_1_OFFSET);
                break;
            case 2:
                aux_state = (BattleAIState *)(
                    gBattleContext + BATTLE_AI_PARTY_STATE_2_OFFSET);
                break;
            case 3:
                aux_state = (BattleAIState *)(
                    gBattleContext + BATTLE_AI_PARTY_STATE_3_OFFSET);
                break;
            case 4:
                aux_state = (BattleAIState *)(
                    gBattleContext + BATTLE_AI_PARTY_STATE_4_OFFSET);
                break;
            default:
                actor_index = (s16)(
                    ((u16)script_target_id & BATTLE_AI_TASK_ACTOR_ID_MASK) -
                    BATTLE_VM_ENEMY_ACTOR_FIRST_ID);
                switch ((u16)script_target_id & BATTLE_AI_TASK_TYPE_MASK) {
                case BATTLE_AI_TASK_ACTION:
                    enemy = *(BattleEnemyActor **)(
                        gBattleContext + BATTLE_VM_ENEMY_ACTOR_TABLE_OFFSET +
                        actor_index * sizeof(void *));
                    aux_state = (BattleAIState *)((u8 *)enemy +
                        BATTLE_VM_ACTOR_ACTION_STATE_OFFSET);
                    break;
                case BATTLE_AI_TASK_REACTION:
                    enemy = *(BattleEnemyActor **)(
                        gBattleContext + BATTLE_VM_ENEMY_ACTOR_TABLE_OFFSET +
                        actor_index * sizeof(void *));
                    aux_state = (BattleAIState *)((u8 *)enemy +
                        BATTLE_VM_ACTOR_REACTION_STATE_OFFSET);
                    break;
                case BATTLE_AI_TASK_AUXILIARY:
                    enemy = *(BattleEnemyActor **)(
                        gBattleContext + BATTLE_VM_ENEMY_ACTOR_TABLE_OFFSET +
                        actor_index * sizeof(void *));
                    aux_state = (BattleAIState *)((u8 *)enemy +
                        BATTLE_VM_ACTOR_AUXILIARY_STATE_OFFSET);
                    break;
                }
                break;
            }

            switch (command->arguments[1]) {
            case BATTLE_VM_SCRIPT_STOP:
                aux_state->script = 0;
                break;
            case BATTLE_VM_SCRIPT_RESUME:
                aux_state->disabled = 0;
                break;
            case BATTLE_VM_SCRIPT_PAUSE:
                aux_state->disabled = 1;
                break;
            }
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_ASSIGN_SCRIPT_OWNER_GROUP:
        ai_state->order = (u16)command->arguments[0] - 3;
        ai_state->flags |= BATTLE_AI_STATE_FLAG_ORDER_WAIT;
        return BATTLE_AI_VM_YIELD;

    case BATTLE_VM_SWAP_OBJECT_SLOTS:
        BattleSceneObject_SwapSlots((u16)command->arguments[0],
                                    (u16)command->arguments[1]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SET_RUNTIME_FLAG_03:
        ((BattleContext *)gBattleContext)->runtime.flags.bits.runtime_flag_03 =
            (u16)command->arguments[0];
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SET_STATE_OVERRIDE:
        *(u32 *)(gBattleContext + BATTLE_VM_STATE_OVERRIDE_OFFSET) =
            (u16)command->arguments[0];
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SET_RESULT_CODE:
        *(u16 *)(gSaveData + BATTLE_VM_SAVE_RESULT_CODE_OFFSET) =
            command->arguments[0];
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_CLEAR_NEXT_STATE:
        *(u32 *)(gBattleContext + BATTLE_VM_NEXT_STATE_OFFSET) = 0;
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_GET_GLOBAL_PROPERTY:
        BattleVm_WriteResult(
            vm, state, command,
            BattleGlobalProperty_Get((u16)command->arguments[0]));
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SET_GLOBAL_PROPERTY:
        BattleGlobalProperty_Set((u16)command->arguments[0],
                                 command->arguments[1]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_FIND_MOST_DAMAGED_ENEMY: {
        int probe_result;
        switch ((u16)command->arguments[0]) {
        case 0:
            BattleActor_FindHighestHpEnemy((u16)command->arguments[1]);
            /* Intentional original fallthrough: the first two probes are
             * discarded by the shipped legacy selector path. */
        case 1:
            BattleActor_FindLowestHpEnemy((u16)command->arguments[1]);
        case 2:
            probe_result = BattleActor_FindMostDamagedEnemy(
                (u16)command->arguments[1]);
            break;
        }
        BattleVm_WriteResult(vm, state, command, probe_result);
        return SCRIPT_VM_CONTINUE;
    }

    case BATTLE_VM_LEGACY_NOOP_049:
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_GET_ENCOUNTER_ID:
        BattleVm_WriteResult(
            vm, state, command,
            *(s16 *)(gSaveData + BATTLE_VM_SAVE_ENCOUNTER_ID_OFFSET));
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_GET_ENCOUNTER_RESOURCE_INDEX:
        BattleVm_WriteResult(
            vm, state, command,
            BattleMap_GetEncounterResourceIndex(command->arguments[0]));
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_LEGACY_NOOP_04C:
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_GET_ACTOR_PROPERTY:
        BattleVm_WriteResult(
            vm, state, command,
            BattleScript_GetProperty((u16)command->arguments[0],
                                     (u16)command->arguments[1]));
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SET_ACTOR_PROPERTY_FIXED:
        if ((command->argument_modes & (1 << 2)) == 0) {
            command->arguments[2] = (s32)BattleVm_PackHalfwords(
                command->arguments[2], command->arguments[3]);
        }
        /* fallthrough */
    case BATTLE_VM_SET_ACTOR_PROPERTY:
        BattleScript_SetProperty(
            (u16)command->arguments[0], (u16)command->arguments[1],
            command->arguments[2]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_ALIGN_OBJECT_VIEW_BASELINE:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        reference = BattleSceneObject_GetById((u16)command->arguments[1]);
        BattleVm_StoreObjectViewPosition(&alignment_position, object);
        z = alignment_position.z;
        BattleVm_StoreObjectViewPosition(&alignment_position, reference);
        object->effect_anchor_z +=
            alignment_position.z - z + command->arguments[2];
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_LEGACY_NOOP_051:
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_APPLY_OBJECT_MOVEMENT:
        object_id = (u16)command->arguments[0];
        BattleSceneObject_ApplyMovement(
            object_id, 1, 0, command->arguments[1],
            command->arguments[2], command->arguments[3], 0, 0, 0, 0);
        BattleSceneObject_ApplyMovement(
            object_id, 1, 0, command->arguments[1],
            command->arguments[2], command->arguments[3], 0, 0, 0, 0);
        if (object_id >= BATTLE_ACTOR_PARTY_FIRST &&
            object_id <
                BATTLE_ACTOR_ENEMY_FIRST + BATTLE_ACTOR_ENEMY_COUNT) {
            actor = BattleActor_GetById(object_id);
            actor->unk_018 = command->arguments[1];
            actor->unk_01a = command->arguments[2];
            actor->unk_01c = command->arguments[3];
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_GET_OBJECT_VIEW_COORDINATE:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleVm_StorePackedObjectViewPosition(&coordinate_position, object);
        switch ((u16)command->arguments[1]) {
        case 0:
            BattleVm_WriteResult(vm, state, command, coordinate_position.x);
            break;
        case 1:
            BattleVm_WriteResult(vm, state, command, coordinate_position.y);
            break;
        case 2:
            BattleVm_WriteResult(vm, state, command, coordinate_position.z);
            break;
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_GET_ENEMY_STAT:
        enemy = (BattleEnemyActor *)BattleActor_GetEnemySlot(
            (u16)command->arguments[0]);
        enemy_stats = BattleEnemy_GetStats((u16)command->arguments[0]);
        if (enemy == 0 || enemy_stats == 0) {
            BattleVm_WriteResult(vm, state, command, 0);
            return SCRIPT_VM_CONTINUE;
        }
        switch ((u16)command->arguments[1]) {
        case 0:
            BattleVm_WriteResult(vm, state, command, enemy_stats->name_id);
            return SCRIPT_VM_CONTINUE;
        case 1:
            BattleVm_WriteResult(
                vm, state, command, (s16)enemy_stats->packed_object_data_id
            );
            return SCRIPT_VM_CONTINUE;
        case 2:
            BattleVm_WriteResult(
                vm, state, command,
                enemy_stats->flag_bits.unknown_flag_0
            );
            return SCRIPT_VM_CONTINUE;
        case 3:
            BattleVm_WriteResult(
                vm, state, command,
                enemy_stats->flag_bits.unknown_flag_1
            );
            return SCRIPT_VM_CONTINUE;
        case 4:
            BattleVm_WriteResult(
                vm, state, command,
                enemy_stats->flag_bits.unknown_flag_2
            );
            return SCRIPT_VM_CONTINUE;
        case 5:
            BattleVm_WriteResult(vm, state, command, enemy_stats->level);
            return SCRIPT_VM_CONTINUE;
        case 6:
            BattleVm_WriteResult(vm, state, command, enemy_stats->max_hp);
            return SCRIPT_VM_CONTINUE;
        case 7:
            BattleVm_WriteResult(vm, state, command, enemy_stats->power);
            return SCRIPT_VM_CONTINUE;
        case 8:
            BattleVm_WriteResult(vm, state, command, enemy_stats->defense);
            return SCRIPT_VM_CONTINUE;
        case 9:
            BattleVm_WriteResult(vm, state, command, enemy_stats->speed);
            return SCRIPT_VM_CONTINUE;
        case 10:
            BattleVm_WriteResult(
                vm, state, command, enemy->state_flag_bits.traits
            );
            return SCRIPT_VM_CONTINUE;
        case 11:
            BattleVm_WriteResult(
                vm, state, command,
                enemy_stats->trait_bits.unknown_trait_02
            );
            return SCRIPT_VM_CONTINUE;
        case 12:
            BattleVm_WriteResult(
                vm, state, command,
                enemy_stats->trait_bits.unknown_trait_03_04
            );
            return SCRIPT_VM_CONTINUE;
        case 13:
            BattleVm_WriteResult(
                vm, state, command,
                enemy_stats->resistance_bits.status_3_resistance
            );
            return SCRIPT_VM_CONTINUE;
        case 14:
            BattleVm_WriteResult(
                vm, state, command,
                enemy_stats->resistance_bits.status_2_resistance
            );
            return SCRIPT_VM_CONTINUE;
        case 15:
            BattleVm_WriteResult(
                vm, state, command,
                enemy_stats->resistance_bits.status_4_5_resistance
            );
            return SCRIPT_VM_CONTINUE;
        case 16:
            BattleVm_WriteResult(
                vm, state, command,
                enemy_stats->resistance_bits.status_6_8_resistance
            );
            return SCRIPT_VM_CONTINUE;
        case 17:
            BattleVm_WriteResult(
                vm, state, command, enemy_stats->experience
            );
            return SCRIPT_VM_CONTINUE;
        case 18:
            BattleVm_WriteResult(vm, state, command, enemy_stats->coins);
            return SCRIPT_VM_CONTINUE;
        case 19:
            BattleVm_WriteResult(
                vm, state, command, enemy_stats->item_drop_1.fields.item_id
            );
            return SCRIPT_VM_CONTINUE;
        case 20:
            BattleVm_WriteResult(
                vm,
                state,
                command,
                enemy_stats->item_drop_1.fields.chance_percent
            );
            return SCRIPT_VM_CONTINUE;
        case 21:
            BattleVm_WriteResult(
                vm, state, command, enemy_stats->item_drop_2.fields.item_id
            );
            return SCRIPT_VM_CONTINUE;
        case 22:
            BattleVm_WriteResult(
                vm,
                state,
                command,
                enemy_stats->item_drop_2.fields.chance_percent
            );
            return SCRIPT_VM_CONTINUE;
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SET_ENEMY_STAT:
        enemy = (BattleEnemyActor *)BattleActor_GetEnemySlot(
            (u16)command->arguments[0]);
        enemy_stats = BattleEnemy_GetStats((u16)command->arguments[0]);
        switch ((u16)command->arguments[1]) {
        case 0:
            enemy_stats->name_id = command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 1:
            enemy_stats->packed_object_data_id = command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 2:
            enemy_stats->flag_bits.unknown_flag_0 = command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 3:
            enemy_stats->flag_bits.unknown_flag_1 = command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 4:
            enemy_stats->flag_bits.unknown_flag_2 = command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 5:
            enemy_stats->level = command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 6:
            enemy_stats->max_hp = command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 7:
            enemy_stats->power = command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 8:
            enemy_stats->defense = command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 9:
            enemy_stats->speed = command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 10:
            enemy->state_flags =
                (enemy->state_flags & ~(3 << 6)) |
                ((command->arguments[2] & 3) << 6);
            return SCRIPT_VM_CONTINUE;
        case 11:
            enemy_stats->trait_bits.unknown_trait_02 =
                command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 12:
            enemy_stats->trait_bits.unknown_trait_03_04 =
                command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 13:
            enemy_stats->resistance_bits.status_3_resistance =
                command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 14:
            enemy_stats->resistance_bits.status_2_resistance =
                command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 15:
            enemy_stats->resistance_bits.status_4_5_resistance =
                command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 16:
            enemy_stats->resistance_bits.status_6_8_resistance =
                command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 17:
            enemy_stats->experience = command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 18:
            enemy_stats->coins = command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 19:
            enemy_stats->item_drop_1.fields.item_id = command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 20:
            enemy_stats->item_drop_1.fields.chance_percent =
                command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 21:
            enemy_stats->item_drop_2.fields.item_id = command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        case 22:
            enemy_stats->item_drop_2.fields.chance_percent =
                command->arguments[2];
            return SCRIPT_VM_CONTINUE;
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_WAIT_OBJECT_PENDING_STATE:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        if (object->flags.pending_state < 0) {
            object->flags.pending_state = 0;
            return SCRIPT_VM_CONTINUE;
        }
        if (object->flags.pending_state == 0) {
            object->flags.pending_state = 1;
        }
        return BattleVm_RetryCurrentCommand(
            vm, state, BATTLE_VM_WAIT_OBJECT_PENDING_STATE
        );

    case BATTLE_VM_SET_ACTIVE_MODEL_ANIMATION:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        model = BattleSceneObject_GetActiveModel(object);
        BattleVm_SetActiveModelAnimation(model, command);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_CONFIGURE_ANIMATION_LAYER:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        model = BattleSceneObject_GetActiveModel(object);
        BattleSceneObject_ConfigureAnimationLayer(
            (u16)command->arguments[0], (u16)command->arguments[1]);
        if (model->animation_controller != 0 &&
            (u16)command->arguments[1] == 0xFFFF) {
            BattleVm_RefreshModelPalette(model);
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SET_ANIMATION_LAYER_STATE: {
        BattleModel *active_model;

        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        active_model = BattleSceneObject_GetActiveModel(object);
        active_model->configure_animation_layer(
            (s8)command->arguments[2],
            (s16)command->arguments[1], 1);
        active_model->animation_layer_states[command->arguments[2]] =
            command->arguments[3];
        if (command->arguments[1] == -1) {
            BattleVm_RefreshModelPalette(active_model);
        }
        return SCRIPT_VM_CONTINUE;
    }

    case BATTLE_VM_INVOKE_MODEL_RELATION_TEST:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        reference = BattleSceneObject_GetById((u16)command->arguments[1]);
        BattleScene_TestModelRelation(
            object, reference, command->arguments[2], command->arguments[3]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_MOVE_OBJECT:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        if ((u16)command->arguments[2] == 0 &&
            command->arguments[7] != 0) {
            command->arguments[3] += object->x;
            command->arguments[4] += object->y;
            command->arguments[5] += object->z;
        }
        switch ((u16)command->arguments[2]) {
        case 0:
            BattleSceneObject_AdjustPosition(
                object, command->arguments[3] - object->x,
                command->arguments[4] - object->y,
                command->arguments[5] - object->z);
            break;
        case 1:
            BattleSceneObject_MoveBy(
                object, (u16)command->arguments[1],
                command->arguments[3], command->arguments[4],
                command->arguments[5], command->arguments[6]);
            break;
        case 2:
            BattleSceneObject_MoveTo(
                object, (u16)command->arguments[1],
                command->arguments[3], command->arguments[4],
                command->arguments[5], command->arguments[6]);
            break;
        case 5:
            reference = BattleSceneObject_GetById(
                (u16)command->arguments[7]);
            BattleSceneObject_MoveToObject(
                object, (u16)command->arguments[1],
                command->arguments[3], command->arguments[4],
                command->arguments[5], command->arguments[6], reference);
            break;
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_KEYFRAMED_MOTION:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        motion_parameters = BattleSceneObject_BeginMotionChannel(
            object, (u16)command->arguments[1], 0,
            BattleMotion_UpdatePath);
        keyframe_record = (const s32 *)(
            (const u8 *)state->script + 2 * command->arguments[3]);
        if (((u32)keyframe_record & 3) != 0) {
            keyframe_record = (const s32 *)((u32)keyframe_record & ~3);
        }
        BattleVm_DecodeFixedArgument(command, 5);
        func_ov002_020724b0(
            motion_parameters, keyframe_record + 1,
            *keyframe_record / 2,
            (command->arguments[4] - 1) << 16, command->arguments[5]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_GET_MOTION_FRAME:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        motion_channel =
            &object->motion_channels[(u16)command->arguments[1]];
        BattleVm_WriteResult(
            vm, state, command,
            (*(s32 *)&motion_channel->parameters[4] >> 16) + 1);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_SINUSOIDAL_DIRECTION_MOTION:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleMotion_StartSineDisplacement(
            object, (u16)command->arguments[1], command->arguments[3],
            command->arguments[4], command->arguments[5],
            command->arguments[6], command->arguments[7],
            command->arguments[8], command->arguments[9]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_MOVE_OBJECT_AT_SPEED:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        if (command->arguments[6] < 1) {
            command->arguments[6] = object->smoothed_travel_distance;
        }
        mode = (u16)command->arguments[2];
        switch (mode) {
        case 1:
            x = command->arguments[3];
            y = command->arguments[4];
            z = command->arguments[5];
            command->arguments[6] = _s32_div_f(
                FX_Sqrt((x * x + y * y + z * z) << 12),
                command->arguments[6]);
            BattleSceneObject_MoveBy(
                object, (u16)command->arguments[1], command->arguments[3],
                command->arguments[4], command->arguments[5],
                command->arguments[6]);
            break;
        case 2:
            command->arguments[6] = _s32_div_f(
                FX_Sqrt(((command->arguments[3] - object->x) *
                             (command->arguments[3] - object->x) +
                         (command->arguments[4] - object->y) *
                             (command->arguments[4] - object->y) +
                         (command->arguments[5] - object->z) *
                         (command->arguments[5] - object->z)) << 12),
                command->arguments[6]);
            BattleSceneObject_MoveTo(
                object, (u16)command->arguments[1], command->arguments[3],
                command->arguments[4], command->arguments[5],
                command->arguments[6]);
            break;
        case 5: {
            int delta_x = command->arguments[3] - object->x;
            int delta_y = command->arguments[4] - object->y;
            int delta_z = command->arguments[5] - object->z;
            reference = BattleSceneObject_GetById(
                (u16)command->arguments[7]);
            delta_x += reference->x;
            delta_y += reference->y;
            delta_z += reference->z;
            command->arguments[6] = _s32_div_f(
                FX_Sqrt((delta_x * delta_x + delta_y * delta_y +
                         delta_z * delta_z) << 12),
                command->arguments[6]);
            BattleSceneObject_MoveToObject(
                reference, (u16)command->arguments[1], command->arguments[3],
                command->arguments[4], command->arguments[5],
                command->arguments[6],
                BattleSceneObject_GetById((u16)command->arguments[7]));
            break;
        }
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_AXIS_KINEMATIC_MOTION:
        BattleVm_StartVerticalMotion(vm, state, command, 0);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_MOVE_WITH_KINEMATIC_DURATION:
        BattleVm_MoveWithVerticalDuration(vm, state, command, 0);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_START_AXIS_KINEMATIC_MOTION_GET_DURATION:
        BattleVm_StartVerticalMotion(vm, state, command, 1);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_MOVE_WITH_KINEMATIC_DURATION_GET_DURATION:
        BattleVm_MoveWithVerticalDuration(vm, state, command, 1);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_START_DIRECTIONAL_KINEMATIC_A:
        BattleVm_StartMotionWithPeakDistance(vm, state, command, 0);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_START_DIRECTIONAL_KINEMATIC_B:
        BattleVm_StartScaledAcceleratedMotion(vm, state, command, 0);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_START_DIRECTIONAL_BALLISTIC:
        BattleVm_StartBallisticMotion(vm, state, command, 0);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_START_DIRECTIONAL_KINEMATIC_A_GET_DURATION:
        BattleVm_StartMotionWithPeakDistance(vm, state, command, 1);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_START_DIRECTIONAL_KINEMATIC_B_GET_DURATION:
        BattleVm_StartScaledAcceleratedMotion(vm, state, command, 1);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_START_DIRECTIONAL_BALLISTIC_GET_DURATION:
        BattleVm_StartBallisticMotion(vm, state, command, 1);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_VELOCITY_MOTION:
        BattleVm_DecodeFixedArgument(command, 7);
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleSceneObject_StartMotionWithVelocityForDuration(
            object, (u16)command->arguments[1], command->arguments[2],
            command->arguments[3], command->arguments[4],
            command->arguments[5], command->arguments[6],
            command->arguments[7]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_ACCELERATION_MOTION:
        BattleVm_DecodeFixedArgument(command, 7);
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleSceneObject_StartMotionWithAccelerationForDuration(
            object, (u16)command->arguments[1], command->arguments[2],
            command->arguments[3], command->arguments[4],
            command->arguments[5], command->arguments[6],
            command->arguments[7]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_ACCELERATED_MOTION:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleSceneObject_StartAcceleratedMotionForDuration(
            object, (u16)command->arguments[1], command->arguments[2],
            command->arguments[3], command->arguments[4],
            command->arguments[5], command->arguments[6],
            command->arguments[7], (u16)command->arguments[8]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_ACCELERATED_MOTION_WITH_RATIO:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleMotion_StartWithVelocityRatio(
            object, (u16)command->arguments[1], command->arguments[2],
            command->arguments[3], command->arguments[4],
            command->arguments[5], command->arguments[6],
            command->arguments[7], (u16)command->arguments[8]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_ACCELERATED_MOTION_BETWEEN_VELOCITIES:
        BattleVm_DecodeFixedArgument(command, 6);
        BattleVm_DecodeFixedArgument(command, 8);
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        func_ov002_020be3e8(
            object, (u16)command->arguments[1], command->arguments[2],
            command->arguments[3], command->arguments[4],
            command->arguments[5], command->arguments[6],
            command->arguments[8], (u16)command->arguments[10]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_ROTATE_OBJECT_AROUND_AXIS:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleMotion_StartAxisRotation(
            object, (u16)command->arguments[1],
            (s16)command->arguments[2], (s16)command->arguments[3],
            (s16)command->arguments[4], (s16)command->arguments[5],
            (s16)command->arguments[6], (s16)command->arguments[7],
            (s16)command->arguments[8], (s16)command->arguments[9]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_WAIT_ANIMATION:
        if (BattleSceneObject_IsAnimationActiveById(
                (u16)command->arguments[0],
                (u16)command->arguments[1])) {
            return BattleVm_RetryCurrentCommand(
                vm, state, BATTLE_VM_WAIT_ANIMATION
            );
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_STOP_MOTION:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleSceneObject_StopMotionChannel(
            object, (u16)command->arguments[1], 0);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SNAPSHOT_POSITION:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        object->motion_origin_x = object->x;
        object->motion_origin_y = object->y;
        object->motion_origin_z = object->z;
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_AUX_SCRIPT:
        switch ((u16)command->arguments[1]) {
        case 0x4000:
            command->arguments[1] =
                BattleActor_SelectRandomStatusTarget(0x38, 2);
            break;
        case 0x8000:
            command->arguments[1] =
                BattleActor_SelectRandomStatusTarget(0x3C, 6);
            break;
        case 0xC000:
            command->arguments[1] =
                BattleActor_SelectRandomStatusTarget(0x38, 8);
            break;
        }
        BattleAI_StartActorAuxScript(
            (u16)command->arguments[0], command->arguments[1],
            state->script + command->arguments[2]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_WAIT_AUX_SCRIPT:
        if (command->arguments[1] != 0) {
            actor_index = (u16)command->arguments[0] -
                          BATTLE_VM_ENEMY_ACTOR_FIRST_ID;
            enemy = *(BattleEnemyActor **)(
                gBattleContext + BATTLE_VM_ENEMY_ACTOR_TABLE_OFFSET +
                actor_index * sizeof(void *));
            aux_state = (BattleAIState *)((u8 *)enemy +
                BATTLE_VM_ACTOR_AUXILIARY_STATE_OFFSET);
            if (aux_state->script != 0) {
                return BattleVm_RetryCurrentCommand(
                    vm, state, BATTLE_VM_WAIT_AUX_SCRIPT
                );
            }
        } else {
            actor_index = (u16)command->arguments[0] -
                          BATTLE_VM_ENEMY_ACTOR_FIRST_ID;
            enemy = *(BattleEnemyActor **)(
                gBattleContext + BATTLE_VM_ENEMY_ACTOR_TABLE_OFFSET +
                actor_index * sizeof(void *));
            aux_state = (BattleAIState *)((u8 *)enemy +
                BATTLE_VM_ACTOR_AUXILIARY_STATE_OFFSET);
            if (aux_state->script != 0) {
                return BattleVm_RetryCurrentCommand(
                    vm, state, BATTLE_VM_WAIT_AUX_SCRIPT
                );
            }
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SUPPRESS_ACTOR_RENDERING:
        actor = BattleActor_GetById((u16)command->arguments[0]);
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        model = BattleSceneObject_GetActiveModel(object);
        if (actor != 0) {
            actor->flags |= BATTLE_ACTOR_FLAG_12;
            actor->scene_object->time_paused = 1;
        }
        if (model != 0) {
            model->flags |= BATTLE_MODEL_FLAG_09;
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_RESTORE_ACTOR_RENDERING:
        actor = BattleActor_GetById((u16)command->arguments[0]);
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        model = BattleSceneObject_GetActiveModel(object);
        if (actor != 0) {
            actor->flags &= ~BATTLE_ACTOR_FLAG_12;
            actor->scene_object->time_paused = 0;
        }
        if (model != 0) {
            model->flags &= ~BATTLE_MODEL_FLAG_09;
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_LEGACY_NOOP_077:
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_REMOVE_ENEMY:
        BattleEnemy_Remove((u16)command->arguments[0],
                           (u16)command->arguments[1]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_LEGACY_NOOP_079:
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_LEGACY_NOOP_07A:
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_LEGACY_NOOP_07B:
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_LEGACY_NOOP_07C:
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_DAMAGE_ACTOR: {
        u16 actor_id = (u16)command->arguments[0];

        object = BattleSceneObject_GetById(actor_id);
        damage = (u16)command->arguments[1];
        if (actor_id >= BATTLE_ACTOR_PARTY_FIRST &&
            actor_id < BATTLE_ACTOR_ENEMY_FIRST) {
            BattleDamage_ApplyToParty(
                object, command->arguments[2], command->arguments[3],
                damage, 0, 0);
        } else if (actor_id >= BATTLE_ACTOR_ENEMY_FIRST &&
                   actor_id < BATTLE_ACTOR_ENEMY_FIRST + 6) {
            BattleActor_ApplyDamage(object, damage);
            BattleVm_StoreObjectViewPosition(&damage_position, object);
            BattleDamage_SpawnNumber(
                damage, damage_position.x + command->arguments[2],
                damage_position.y + command->arguments[3], 7, 0);
        }
        return SCRIPT_VM_CONTINUE;
    }

    case BATTLE_VM_HEAL_ACTOR:
        actor = BattleActor_GetById((u16)command->arguments[0]);
        damage = (u16)command->arguments[1];
        object = actor->scene_object;
        if (actor->max_hp - actor->current_hp < damage) {
            damage = actor->max_hp - actor->current_hp;
        }
        actor->current_hp += damage;
        model_effect = (BattleModelEffect *)BattleModelEffect_Spawn(
            10, 0,
            (s16)(object->x + command->arguments[2]),
            (s16)(object->y - object->z + command->arguments[3]),
            0, 256);
        model_effect->user_value = damage;
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_APPLY_STATUS:
        actor = BattleActor_GetById((u16)command->arguments[0]);
        BattleVm_WriteResult(
            vm, state, command,
            BattleStatus_TryApply(
                actor, command->arguments[1],
                *(s16 *)(gBattleContext + BATTLE_VM_STATUS_DURATION_OFFSET),
                command->arguments[3], command->arguments[2]));
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_CLEAR_STATUS:
        actor = BattleActor_GetById((u16)command->arguments[0]);
        if (command->arguments[1] == -1) {
            BattleStatus_ClearAll(actor);
        } else {
            BattleStatus_ClearEffect(actor, command->arguments[1]);
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_WAIT_HIT_QUEUE:
        if (BattleActor_IsAnyHitLocked()) {
            return BattleVm_RetryCurrentCommand(
                vm, state, BATTLE_VM_WAIT_HIT_QUEUE
            );
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_CALCULATE_EQUIPMENT_DAMAGE:
        damage = BattleDamage_ApplyEquipmentModifiers(
            (u16)command->arguments[0], (u16)command->arguments[1],
            (command->arguments[2] << 8) / 100,
            command->arguments[3]);
        BattleVm_WriteResult(vm, state, command, damage);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SET_ACTOR_TARGETING_ENABLED:
        actor = BattleActor_GetById((u16)command->arguments[0]);
        actor->flag_bits.excluded_from_targeting =
            (u16)(int)(command->arguments[1] == 0);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_CONFIGURE_HIT:
        hit_descriptor = BattleHitDescriptor_Configure(
            command->arguments[0], command->arguments[1], 0,
            command->arguments[2], BATTLE_HIT_KIND_DEFAULT);
        BattleHitDescriptor_SetStatus(
            hit_descriptor, command->arguments[3], command->arguments[4],
            command->arguments[5]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_REFLECT_QUEUED_HITS:
        BattleDamage_ReflectQueuedHits((u16)command->arguments[0]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_FIND_HIT_DESCRIPTOR: {
        int source_id;
        int target_id;
        BattleHitRecord *record = (BattleHitRecord *)(
            gBattleContext + BATTLE_VM_HIT_QUEUE_OFFSET);
        int record_index = 0;

        for (;;) {
            if (record->kind == 0) {
                BattleVm_WriteResult(vm, state, command, 0);
                return SCRIPT_VM_CONTINUE;
            }
            source_id = record->source_id;
            target_id = record->target_id;
            if (source_id == 8) source_id = BATTLE_ACTOR_MARIO;
            if (source_id == 9) source_id = BATTLE_ACTOR_LUIGI;
            if (target_id == 8) target_id = BATTLE_ACTOR_MARIO;
            if (target_id == 9) target_id = BATTLE_ACTOR_LUIGI;
            if (source_id == command->arguments[0] &&
                target_id == command->arguments[1]) {
                BattleVm_WriteResult(
                    vm, state, command, record_index + 1);
                return SCRIPT_VM_CONTINUE;
            }
            record++;
            record_index++;
        }
    }

    case BATTLE_VM_GET_QUEUED_HIT_X:
    case BATTLE_VM_GET_QUEUED_HIT_Y:
    case BATTLE_VM_GET_QUEUED_HIT_Z:
    case BATTLE_VM_GET_QUEUED_HIT_KIND: {
        int hit_value;

        hit_record = (BattleHitRecord *)(
            gBattleContext + BATTLE_VM_HIT_QUEUE_OFFSET) +
            (command->arguments[0] - 1);
        switch (command->opcode) {
        case BATTLE_VM_GET_QUEUED_HIT_X:
            hit_value = hit_record->x;
            break;
        case BATTLE_VM_GET_QUEUED_HIT_Y:
            hit_value = hit_record->y;
            break;
        case BATTLE_VM_GET_QUEUED_HIT_Z:
            hit_value = hit_record->z;
            break;
        case BATTLE_VM_GET_QUEUED_HIT_KIND:
            hit_value = hit_record->kind;
            break;
        }
        BattleVm_WriteResult(vm, state, command, hit_value);
        return SCRIPT_VM_CONTINUE;
    }

    case BATTLE_VM_DAMAGE_PARTY:
        party_actor = (BattlePartyActor *)BattleActor_GetPartySlot(
            (u16)command->arguments[0]);
        mode = command->arguments[2] & 0x7FFF;
        switch (mode) {
        case 0:
            damage = 1;
            break;
        case 1:
            damage = BattleDamage_CalculateByObject(
                (u16)command->arguments[3],
                party_actor->actor.scene_object->actor_id);
            break;
        case 2:
            damage = command->arguments[3];
            break;
        }
        if (command->arguments[1] == -1) {
            BattleDamage_ApplyToParty(
                party_actor->actor.scene_object, command->arguments[4],
                command->arguments[5], damage, 2, 0);
            BattleDamage_StartPartyLaunchReaction(&party_actor->actor);
        } else {
            BattleVm_StorePackedObjectViewPosition(
                &party_damage_position,
                party_actor->actor.scene_object);
            if ((command->arguments[2] & 0x8000) == 0) {
                BattleModelEffect_Spawn(
                    27, 0, party_damage_position.x,
                    party_damage_position.y, party_damage_position.z, 256);
                BattleSpriteEffect_Spawn(
                    12, party_damage_position.x, party_damage_position.y,
                    party_damage_position.z, 256);
            }
            func_ov002_02077e78(
                &party_actor->actor, (s16)command->arguments[1], damage,
                command->arguments[4], command->arguments[5]);
        }
        *(u16 *)party_actor->unknown_070 = 0;
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SET_AUXILIARY_OBJECTS_PAUSED:
        BattleScene_SetAuxiliaryObjectsPaused(command->arguments[0]);
        break;

    case BATTLE_VM_CALCULATE_DAMAGE:
        BattleVm_WriteResult(
            vm, state, command,
            BattleDamage_CalculateByObject(
                (u16)command->arguments[0],
                (u16)command->arguments[1]));
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SPAWN_MODEL_EFFECT:
        z = (s16)((s16)command->arguments[3] -
                  (s16)command->arguments[4]);
        y = (s16)command->arguments[3];
        x = (s16)((s16)command->arguments[5] +
                  16 * (256 - y));
        y = (s16)((s16)command->arguments[2] - *(s16 *)(
            gBattleContext + BATTLE_VM_EFFECT_VIEW_X_OFFSET));
        z = (s16)(z - *(s16 *)(
            gBattleContext + BATTLE_VM_EFFECT_VIEW_Y_OFFSET));
        if (x < 0) {
            x = 0;
        }
        BattleModelEffect_SpawnFromResource(
            (u16)command->arguments[0], (u16)command->arguments[1],
            y, z, (s16)x,
            command->arguments[6] / 16);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SPAWN_ATTACHED_MODEL_EFFECT:
        z = (s16)command->arguments[5];
        y = (s16)-command->arguments[4];
        x = (s16)command->arguments[3];
        object = BattleSceneObject_GetById((u16)command->arguments[2]);
        BattlePosition_StoreViewRelative(
            (BattlePosition *)&attached_model_position,
            (s16)(object->x + x),
            (s16)(object->y - (s16)(object->z + y)),
            (s16)((s16)(object->effect_anchor_z + z) +
                  16 * (256 - object->y)),
            object->flags.bits.use_raw_position,
            object->flags.bits.use_alternate_model);
        BattleModelEffect_SpawnFromResource(
            (u16)command->arguments[0], (u16)command->arguments[1],
            attached_model_position.x, attached_model_position.y,
            attached_model_position.z,
            command->arguments[6] / 16);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SPAWN_MODEL_EFFECT_HANDLE:
        z = (s16)command->arguments[5];
        y = (s16)command->arguments[4];
        x = (s16)command->arguments[3];
        object = BattleSceneObject_GetById((u16)command->arguments[2]);
        BattlePosition_StoreViewRelative(
            (BattlePosition *)&model_handle_position,
            (s16)(object->x + x),
            (s16)((s16)(object->y + y) - object->z),
            (s16)((s16)(object->effect_anchor_z + z) +
                  16 * (256 -
                        (s16)(object->y + y))),
            object->flags.bits.use_raw_position,
            object->flags.bits.use_alternate_model);
        effect_index = BattleModelEffect_SpawnFromResourceInFreeSlot(
            (u16)command->arguments[0], (u16)command->arguments[1],
            model_handle_position.x, model_handle_position.y,
            model_handle_position.z,
            command->arguments[6] / 16);
        BattleVm_WriteResult(
            vm, state, command,
            effect_index | BATTLE_VM_MODEL_EFFECT_HANDLE_TAG);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SPAWN_ALTERNATE_MODEL_EFFECT_HANDLE:
        z = (s16)command->arguments[5];
        y = (s16)-command->arguments[4];
        x = (s16)command->arguments[3];
        object = BattleSceneObject_GetById((u16)command->arguments[2]);
        BattlePosition_StoreViewRelative(
            (BattlePosition *)&alternate_model_handle_position,
            (s16)(object->x + x),
            (s16)(object->y - (s16)(object->z + y)),
            (s16)((s16)(object->effect_anchor_z + z) +
                  16 * (256 - object->y)),
            object->flags.bits.use_raw_position,
            object->flags.bits.use_alternate_model);
        effect_index = BattleModelEffect_SpawnFromResourceInFreeSlot(
            (u16)command->arguments[0], (u16)command->arguments[1],
            alternate_model_handle_position.x,
            alternate_model_handle_position.y,
            alternate_model_handle_position.z,
            command->arguments[6] / 16);
        BattleVm_WriteResult(
            vm, state, command,
            effect_index | BATTLE_VM_MODEL_EFFECT_HANDLE_TAG);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SPAWN_SPRITE_EFFECT: {
        int effect_x;
        s16 effect_y;
        s16 effect_z;

        effect_y = (s16)command->arguments[2];
        effect_z = (s16)(effect_y - (s16)command->arguments[3]);
        effect_x = (s16)((s16)command->arguments[4] +
                         16 * (256 - effect_y));
        effect_y = (s16)((s16)command->arguments[1] - *(s16 *)(
            gBattleContext + BATTLE_VM_EFFECT_VIEW_X_OFFSET));
        effect_z = (s16)(effect_z - *(s16 *)(
            gBattleContext + BATTLE_VM_EFFECT_VIEW_Y_OFFSET));
        if (effect_x < 0) {
            effect_x = 0;
        }
        BattleSpriteEffect_Spawn(
            (u16)command->arguments[0],
            effect_y, effect_z, (s16)effect_x,
            command->arguments[5] / 16);
        return SCRIPT_VM_CONTINUE;
    }

    case BATTLE_VM_SPAWN_ATTACHED_SPRITE_EFFECT:
        z = (s16)command->arguments[4];
        y = (s16)-command->arguments[3];
        x = (s16)command->arguments[2];
        object = BattleSceneObject_GetById((u16)command->arguments[1]);
        BattlePosition_StoreViewRelative(
            (BattlePosition *)&attached_sprite_position,
            (s16)(object->x + x),
            (s16)(object->y - (s16)(object->z + y)),
            (s16)((s16)(object->effect_anchor_z + z) +
                  16 * (256 - object->y)),
            object->flags.bits.use_raw_position,
            object->flags.bits.use_alternate_model);
        BattleSpriteEffect_Spawn(
            (u16)command->arguments[0], attached_sprite_position.x,
            attached_sprite_position.y, attached_sprite_position.z,
            command->arguments[5] / 16);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SPAWN_SPRITE_EFFECT_HANDLE: {
        int effect_x;
        s16 effect_y;
        s16 effect_z;

        effect_y = (s16)command->arguments[2];
        effect_z = (s16)(effect_y - (s16)command->arguments[3]);
        effect_x = (s16)((s16)command->arguments[4] +
                         16 * (256 - effect_y));
        effect_y = (s16)((s16)command->arguments[1] - *(s16 *)(
            gBattleContext + BATTLE_VM_EFFECT_VIEW_X_OFFSET));
        effect_z = (s16)(effect_z - *(s16 *)(
            gBattleContext + BATTLE_VM_EFFECT_VIEW_Y_OFFSET));
        if (effect_x < 0) {
            effect_x = 0;
        }
        effect_index = BattleSpriteEffect_SpawnInFreeSlot(
            (u16)command->arguments[0],
            effect_y, effect_z, (s16)effect_x,
            command->arguments[5] / 16);
        BattleVm_WriteResult(
            vm, state, command,
            effect_index | BATTLE_VM_SPRITE_EFFECT_HANDLE_TAG);
        return SCRIPT_VM_CONTINUE;
    }

    case BATTLE_VM_SPAWN_ATTACHED_SPRITE_EFFECT_HANDLE:
        z = (s16)command->arguments[4];
        y = (s16)-command->arguments[3];
        x = (s16)command->arguments[2];
        object = BattleSceneObject_GetById((u16)command->arguments[1]);
        BattlePosition_StoreViewRelative(
            (BattlePosition *)&attached_sprite_handle_position,
            (s16)(object->x + x),
            (s16)(object->y - (s16)(object->z + y)),
            (s16)((s16)(object->effect_anchor_z + z) +
                  16 * (256 - object->y)),
            object->flags.bits.use_raw_position,
            object->flags.bits.use_alternate_model);
        effect_index = BattleSpriteEffect_SpawnInFreeSlot(
            (u16)command->arguments[0], attached_sprite_handle_position.x,
            attached_sprite_handle_position.y,
            attached_sprite_handle_position.z,
            command->arguments[5] / 16);
        BattleVm_WriteResult(
            vm, state, command,
            effect_index | BATTLE_VM_SPRITE_EFFECT_HANDLE_TAG);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SET_ATTACHED_EFFECT_USER_VALUE: {
        u16 effect_handle = command->arguments[0];
        u16 user_value = command->arguments[1];

        effect_slots = (BattleEffect **)(
            gBattleContext + BATTLE_VM_ATTACHED_EFFECT_SLOTS_OFFSET);
        effect = effect_slots[
            effect_handle & BATTLE_VM_HANDLE_INDEX_MASK];
        effect->user_value =
            (effect->user_value & 0xFFFF0000) |
            (user_value & 0xFFFF);
        return SCRIPT_VM_CONTINUE;
    }

    case BATTLE_VM_SPAWN_MODEL_EFFECT_AT_WORLD_POSITION:
        func_ov002_02079090(vm, state, command, 0x11);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_SPAWN_MODEL_EFFECT_AT_OBJECT_POSITION:
        func_ov002_02079090(vm, state, command, 0x12);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_SPAWN_MODEL_EFFECT_AT_WORLD_POSITION_HANDLE:
        func_ov002_02079090(vm, state, command, 0x21);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_SPAWN_MODEL_EFFECT_AT_OBJECT_POSITION_HANDLE:
        func_ov002_02079090(vm, state, command, 0x22);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_LEGACY_NOOP_09B:
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_LEGACY_NOOP_09C:
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SET_TAGGED_EFFECT_FLAG:
        handle = (u16)command->arguments[0];
        handle_index = handle & BATTLE_VM_HANDLE_INDEX_MASK;
        handle_tag = handle & BATTLE_VM_HANDLE_TAG_MASK;
        switch (handle_tag) {
        case BATTLE_VM_MODEL_EFFECT_HANDLE_TAG:
            break;
        case BATTLE_VM_SPRITE_EFFECT_HANDLE_TAG:
            effect_slots = (BattleEffect **)(
                gBattleContext + BATTLE_VM_SPRITE_EFFECT_SLOTS_OFFSET);
            effect = effect_slots[handle_index];
            effect->script_flag = (u16)(int)(command->arguments[1] != 0);
            break;
        case BATTLE_VM_ATTACHED_EFFECT_HANDLE_TAG:
            effect_slots = (BattleEffect **)(
                gBattleContext + BATTLE_VM_ATTACHED_EFFECT_SLOTS_OFFSET);
            effect = effect_slots[handle_index];
            effect->attached_script_flag =
                (u16)(int)(command->arguments[1] != 0);
            break;
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_CLEAR_TAGGED_EFFECT_STATE:
        handle = (u16)command->arguments[0];
        handle_index = handle & BATTLE_VM_HANDLE_INDEX_MASK;
        handle_tag = handle & BATTLE_VM_HANDLE_TAG_MASK;
        switch (handle_tag) {
        case BATTLE_VM_MODEL_EFFECT_HANDLE_TAG:
            break;
        case BATTLE_VM_SPRITE_EFFECT_HANDLE_TAG:
            effect_slots = (BattleEffect **)(
                gBattleContext + BATTLE_VM_SPRITE_EFFECT_SLOTS_OFFSET);
            effect_slots[handle_index]->update_callback = 0;
            break;
        case BATTLE_VM_ATTACHED_EFFECT_HANDLE_TAG:
            effect_slots = (BattleEffect **)(
                gBattleContext + BATTLE_VM_ATTACHED_EFFECT_SLOTS_OFFSET);
            effect_slots[handle_index]->update_callback = 0;
            break;
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_WAIT_SCRIPT_HANDLE:
        if (BattleScriptHandle_IsActive((u16)command->arguments[0])) {
            return BattleVm_RetryCurrentCommand(
                vm, state, BATTLE_VM_WAIT_SCRIPT_HANDLE
            );
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_IS_SCRIPT_HANDLE_ACTIVE:
        BattleVm_WriteResult(
            vm, state, command,
            BattleScriptHandle_IsActive((u16)command->arguments[0]));
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_STRETCH_OBJECT_BETWEEN_ANCHORS:
        BattleSceneObject_StretchBetweenAnchors(
            (s16)command->arguments[0], (s16)command->arguments[1],
            (s16)command->arguments[2], (s16)command->arguments[3],
            (s16)command->arguments[4], (s16)command->arguments[5],
            (s16)command->arguments[6], (s16)command->arguments[7],
            (s16)command->arguments[8], (s16)command->arguments[9],
            (s16)command->arguments[10], (u16)command->arguments[11],
            (u16)command->arguments[12], (u16)command->arguments[13]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_IMPACT_TRAIL_EMITTER:
        BattleImpactEmitter_Start(
            (u16)command->arguments[0], 0,
            command->arguments[1], command->arguments[2],
            command->arguments[3], command->arguments[4],
            command->arguments[5], command->arguments[6],
            command->arguments[7], 0);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_STOP_IMPACT_TRAIL_EMITTER:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        object->flags.bits.stop_impact_particles = 1;
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_INTERPOLATED_OBJECT_LINK:
        func_ov002_020bccc4(
            (u16)command->arguments[0], (u16)command->arguments[1],
            (u16)command->arguments[2], (u16)command->arguments[3],
            (s8)command->arguments[4], (s8)command->arguments[5],
            (s8)command->arguments[6], (s8)command->arguments[7],
            (s8)command->arguments[8], (s8)command->arguments[9],
            (u16)command->arguments[10], 1);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_STOP_INTERPOLATED_OBJECT_LINK:
        BattleLinkEffect_DetachTarget();
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_CANCEL_INTERPOLATED_OBJECT_LINK:
        BattleLinkEffect_Stop();
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_SCREEN_PARTICLE_SWEEP:
        BattleParticleSweep_CreateEmitter(
            command->arguments[0], command->arguments[1],
            command->arguments[2], command->arguments[3],
            (u16)command->arguments[4], (u8)command->arguments[5],
            (u16)command->arguments[6]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_SCREEN_PARTICLE_SWEEP_HANDLE:
        slot_index = 0;
        for (;;) {
            if (*(BattleAITask **)(gBattleContext +
                    slot_index * sizeof(void *) +
                    BATTLE_VM_TASK_HANDLE_SLOTS_OFFSET) == 0) {
                break;
            }
            slot_index++;
        }
        task = BattleParticleSweep_CreateEmitter(
            command->arguments[0], command->arguments[1],
            command->arguments[2], command->arguments[3],
            (u16)command->arguments[4], (u8)command->arguments[5],
            (u16)command->arguments[6]);
        BattleTask_BindOwnerSlot(
            task,
            &((BattleAITask **)(gBattleContext +
                BATTLE_VM_TASK_HANDLE_SLOTS_OFFSET))[slot_index]
        );
        BattleVm_WriteResult(
            vm, state, command,
            slot_index | BATTLE_VM_TASK_HANDLE_TAG);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_GROUND_RIPPLE:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleGroundRipple_Create(
            object, command->arguments[1], command->arguments[2],
            command->arguments[3]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_GROUND_RIPPLE_HANDLE:
        slot_index = 0;
        for (;;) {
            if (*(BattleAITask **)(gBattleContext +
                    slot_index * sizeof(void *) +
                    BATTLE_VM_TASK_HANDLE_SLOTS_OFFSET) == 0) {
                break;
            }
            slot_index++;
        }
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        task = BattleGroundRipple_Create(
            object, command->arguments[1], command->arguments[2],
            command->arguments[3]);
        BattleTask_BindOwnerSlot(
            task,
            &((BattleAITask **)(gBattleContext +
                BATTLE_VM_TASK_HANDLE_SLOTS_OFFSET))[slot_index]
        );
        BattleVm_WriteResult(
            vm, state, command,
            slot_index | BATTLE_VM_TASK_HANDLE_TAG);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_DRAW_PROCEDURAL_GROUND_RIPPLES:
        BattleVm_WriteResult(
            vm, state, command,
            func_ov002_020ae9c0(
                command->arguments[0], command->arguments[1],
                command->arguments[2], command->arguments[3],
                command->arguments[4], command->arguments[5]));
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SCATTER_MODEL_PARTS:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        func_ov002_020bba60(object);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_MODEL_CRUSH:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleMesh_CreateHitTask(
            object, (u16)command->arguments[1], command->arguments[2]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_GET_MODEL_PART_EFFECT_STATE:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleVm_WriteResult(
            vm, state, command, BattleMesh_GetTaskState(object));
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_TRANSFORM_MODEL_EFFECT_BASIS:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleMesh_RotateAndNormalize(
            object, command->arguments[1], (u16)command->arguments[2],
            4096);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_FLEXIBLE_OBJECT_LINK:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        reference = BattleSceneObject_GetById((u16)command->arguments[1]);
        func_ov002_020bb00c(
            object, reference, command->arguments[2], command->arguments[3],
            command->arguments[4], command->arguments[5],
            command->arguments[6]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_GET_FLEXIBLE_LINK_HEADING:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleVm_WriteResult(
            vm, state, command,
            func_ov002_020badd8(object, command->arguments[1]));
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_SEGMENTED_OBJECT_LINK:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        reference = BattleSceneObject_GetById((u16)command->arguments[1]);
        BattleObjectLink_InitializeSegments(
            object, reference, command->arguments[2], command->arguments[3],
            command->arguments[4], command->arguments[5],
            command->arguments[6], command->arguments[7]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_INITIALIZE_SPRITE_GRID_CAPTURE:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleSpriteGridCapture_Initialize(object);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_SPRITE_GRID_CAPTURE:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleSpriteGridCapture_Start(
            object, command->arguments[1], command->arguments[2],
            command->arguments[3], command->arguments[4]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_ADVANCE_SPRITE_GRID_CAPTURE:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleSpriteGridCapture_Advance(
            object, command->arguments[1], command->arguments[2]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_FINISH_SPRITE_GRID_CAPTURE:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleSpriteGridCapture_Finish(
            object, command->arguments[1], command->arguments[2]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_IS_SPRITE_GRID_CAPTURE_ACTIVE:
        object = BattleSceneObject_GetById((u16)command->arguments[0]);
        BattleVm_WriteResult(
            vm, state, command, BattleSpriteGridCapture_IsActive(object));
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SPAWN_OBJECT_DATA_EFFECT_AT_WORLD_POSITION:
        func_ov002_02078e38(vm, state, command, 0x11);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_SPAWN_OBJECT_DATA_EFFECT_AT_OBJECT_POSITION:
        func_ov002_02078e38(vm, state, command, 0x12);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_SPAWN_OBJECT_DATA_EFFECT_AT_WORLD_POSITION_HANDLE:
        func_ov002_02078e38(vm, state, command, 0x21);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_SPAWN_OBJECT_DATA_EFFECT_AT_OBJECT_POSITION_HANDLE:
        func_ov002_02078e38(vm, state, command, 0x22);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_CONFIGURE_GLOBAL_RESOURCE_SLOTS:
        resource_sources[0] = 0;
        resource_sources[1] = 0;
        resource_sources[2] = 0;
        resource_sources[3] = 0;
        BattleModelAnimation_ResolveSource(
            command->arguments[0], &resource_sources[0]);
        BattleModelAnimation_ResolveSource(
            command->arguments[1], &resource_sources[1]);
        BattleModelAnimation_ResolveSource(
            command->arguments[2], &resource_sources[2]);
        BattleModelAnimation_ResolveSource(
            command->arguments[3], &resource_sources[3]);
        BattleModelAnimation_SetModels(
            resource_sources[0], resource_sources[1],
            resource_sources[2], resource_sources[3]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SPAWN_ARCHIVE_EFFECT_AT_WORLD_POSITION:
        func_ov002_02078bb4(vm, state, command, 0x11);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_SPAWN_ARCHIVE_EFFECT_AT_OBJECT_POSITION:
        func_ov002_02078bb4(vm, state, command, 0x12);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_SPAWN_ARCHIVE_EFFECT_AT_WORLD_POSITION_HANDLE:
        func_ov002_02078bb4(vm, state, command, 0x21);
        return SCRIPT_VM_CONTINUE;
    case BATTLE_VM_SPAWN_ARCHIVE_EFFECT_AT_OBJECT_POSITION_HANDLE:
        func_ov002_02078bb4(vm, state, command, 0x22);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_RASTER_RESOURCE_TRANSITION: {
        int task_slot;

        task_slot = 0;
        for (;;) {
            if (*(BattleAITask **)(gBattleContext +
                    task_slot * sizeof(void *) +
                    BATTLE_VM_TASK_HANDLE_SLOTS_OFFSET) == 0) {
                break;
            }
            task_slot++;
        }
        task = (BattleAITask *)BattleRasterResourceTransition_Start(
            command->arguments[0], (u16)command->arguments[1],
            command->arguments[2], command->arguments[3]);
        BattleTask_BindOwnerSlot(
            task,
            &((BattleAITask **)(gBattleContext +
                BATTLE_VM_TASK_HANDLE_SLOTS_OFFSET))[task_slot]
        );
        BattleVm_WriteResult(
            vm, state, command,
            task_slot | BATTLE_VM_TASK_HANDLE_TAG);
        break;
    }

    case BATTLE_VM_START_FINITE_RASTER_EFFECT:
        if (command->arguments[0] == 0) {
            command->arguments[0] = 4;
        } else if (command->arguments[0] == 1) {
            command->arguments[0] = -4;
        }
        BattleRasterEffect_StartFinite(
            command->arguments[0], (u16)command->arguments[1],
            command->arguments[2], command->arguments[3],
            command->arguments[4]);
        break;

    case BATTLE_VM_LEGACY_NOOP_0C3:
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_YIELD_ONE_FRAME:
        return BATTLE_AI_VM_YIELD;

    case BATTLE_VM_SET_SUB_SCREEN_BLEND_COEFFICIENTS:
        *(u16 *)(gBattleContext + BATTLE_VM_SUB_SCREEN_BLEND_EVA_OFFSET) =
            command->arguments[0];
        *(u16 *)(gBattleContext + BATTLE_VM_SUB_SCREEN_BLEND_EVB_OFFSET) =
            command->arguments[1];
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SET_SUB_SCREEN_BLEND_PLANES:
        *(u16 *)(gBattleContext +
                 BATTLE_VM_SUB_SCREEN_BLEND_FIRST_PLANES_OFFSET) =
            command->arguments[0];
        *(u16 *)(gBattleContext +
                 BATTLE_VM_SUB_SCREEN_BLEND_SECOND_PLANES_OFFSET) =
            command->arguments[1];
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_ADJUST_INVENTORY:
        x = GameInventory_Add(
            (u16)command->arguments[0], command->arguments[1]);
        BattleVm_WriteResult(vm, state, command, x);
        BattleItemList_RebuildActionItems();
        BattleItemList_RebuildUsableItems();
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_LEGACY_NOOP_0CA:
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_GET_INVENTORY_COUNT:
        BattleVm_WriteResult(
            vm, state, command, GameInventory_GetCount((u16)command->arguments[0]));
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_GET_ITEM_BATTLE_ANIMATION: {
        const u8 *item_record = BattleVm_GetItemRecord(
            (u16)command->arguments[0]);
        BattleVm_WriteResult(
            vm, state, command,
            item_record != 0 ? item_record[9] : -1);
        return SCRIPT_VM_CONTINUE;
    }

    case BATTLE_VM_CONFIGURE_CONTROL_STATE:
        switch ((u16)command->arguments[0]) {
        case 0:
            BattleAI_UpdateControlMask((u16)command->arguments[1], 1);
            break;
        case 1:
            *(u16 *)(gBattleContext + BATTLE_VM_CONTROL_STATE_OFFSET) =
                command->arguments[1];
            break;
        case 2:
        case 3:
        default:
            break;
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_UPDATE_CONTROL_MASK:
        BattleAI_UpdateControlMask(
            (u16)command->arguments[0], (u16)command->arguments[1]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SET_INPUT_DISABLE_MASK:
        *(u16 *)(gBattleContext + BATTLE_VM_INPUT_DISABLE_MASK_OFFSET) =
            command->arguments[0];
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_SET_INPUT_DIRECTION_ROTATION:
        *(u16 *)(gBattleContext +
                 BATTLE_VM_INPUT_DIRECTION_ROTATION_OFFSET) =
            command->arguments[0];
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_LEGACY_NOOP_0CF:
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_LEGACY_NOOP_0D0:
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_INLINE_OBJECT_SCRIPT:
        BattleAI_StartObjectScript(
            (u16)command->arguments[0], state->script, ai_state,
            (u16)command->arguments[1]);
        state->script += command->arguments[2];
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_INLINE_OBJECT_SCRIPT_AND_YIELD:
        script = (const u16 *)state->script;
        BattleAI_StartObjectScript(
            (u16)command->arguments[0], script + command->arguments[2],
            ai_state, (u16)command->arguments[1]);
        return BATTLE_AI_VM_YIELD;

    case BATTLE_VM_WAIT_INLINE_OBJECT_SCRIPT_AND_SKIP:
        object_script_state = BattleScriptState_GetByObjectId(
            (u16)command->arguments[0]);
        if (object_script_state->script != 0) {
            return BattleVm_RetryCurrentCommand(
                vm, state, BATTLE_VM_WAIT_INLINE_OBJECT_SCRIPT_AND_SKIP
            );
        }
        state->script =
            (const u16 *)state->script + command->arguments[1];
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_START_RELATIVE_OBJECT_SCRIPT:
        script = (const u16 *)state->script;
        BattleAI_StartObjectScript(
            (u16)command->arguments[0], script + command->arguments[2],
            ai_state, (u16)command->arguments[1]);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_WAIT_OBJECT_SCRIPT:
        object_script_state = BattleScriptState_GetByObjectId(
            (u16)command->arguments[0]);
        if (object_script_state->script != 0) {
            return BattleVm_RetryCurrentCommand(
                vm, state, BATTLE_VM_WAIT_OBJECT_SCRIPT
            );
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_WAIT_OBJECT_SCRIPTS_BY_OWNER: {
        BattleAIState *matching_script;
        s16 remaining;

        if (command->arguments[0] < 1) {
            command->arguments[0] = ai_state->owner_id;
        }
        matching_script = (BattleAIState *)(
            gBattleContext + BATTLE_VM_OBJECT_SCRIPT_STATES_OFFSET);
        if (command->arguments[0] < BATTLE_AI_TASK_OBJECT) {
            remaining = BATTLE_VM_OBJECT_SCRIPT_STATE_COUNT;
            do {
                if (command->arguments[0] ==
                        matching_script->order_value &&
                    matching_script->script != 0) {
                    return BattleVm_RetryCurrentCommand(
                        vm, state, BATTLE_VM_WAIT_OBJECT_SCRIPTS_BY_OWNER
                    );
                }
                remaining = (s16)(remaining - 1);
                matching_script++;
            } while (remaining != 0);
        } else {
            s16 actor_remaining = BATTLE_VM_OBJECT_SCRIPT_STATE_COUNT;
            int actor_owner =
                command->arguments[0] & BATTLE_AI_TASK_ACTOR_ID_MASK;

            do {
                if (actor_owner == matching_script->order_tie_break &&
                    matching_script->script != 0) {
                    return BattleVm_RetryCurrentCommand(
                        vm, state, BATTLE_VM_WAIT_OBJECT_SCRIPTS_BY_OWNER
                    );
                }
                actor_remaining = (s16)(actor_remaining - 1);
                matching_script++;
            } while (actor_remaining != 0);
        }
        return SCRIPT_VM_CONTINUE;
    }

    case BATTLE_VM_STOP_OBJECT_SCRIPT:
        BattleScriptState_GetByObjectId(
            (u16)command->arguments[0])->script = 0;
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_STOP_OBJECT_SCRIPTS_BY_OWNER: {
        BattleAIState *matching_script;
        int owner_selector;
        s16 remaining;

        if (command->arguments[0] < 1) {
            command->arguments[0] = ai_state->owner_id;
        }
        owner_selector = command->arguments[0];
        matching_script = (BattleAIState *)(
            gBattleContext + BATTLE_VM_OBJECT_SCRIPT_STATES_OFFSET);
        if (owner_selector < BATTLE_AI_TASK_OBJECT) {
            remaining = BATTLE_VM_OBJECT_SCRIPT_STATE_COUNT;
            do {
                if (command->arguments[0] ==
                        matching_script->order_value &&
                    matching_script->script != 0) {
                    BattleScriptState_GetByObjectId(
                        command->arguments[0])->script = 0;
                    BattleScriptState_GetByObjectId(
                        command->arguments[0])->scratch_a8 = 0;
                }
                remaining = (s16)(remaining - 1);
                matching_script++;
            } while (remaining > 0);
        } else {
            s16 actor_remaining = BATTLE_VM_OBJECT_SCRIPT_STATE_COUNT;
            int actor_owner =
                owner_selector & BATTLE_AI_TASK_ACTOR_ID_MASK;

            do {
                if (actor_owner == matching_script->order_tie_break &&
                    matching_script->script != 0) {
                    BattleScriptState_GetByObjectId(
                        command->arguments[0])->script = 0;
                    BattleScriptState_GetByObjectId(
                        command->arguments[0])->scratch_a8 = 0;
                    break;
                }
                actor_remaining = (s16)(actor_remaining - 1);
                matching_script++;
            } while (actor_remaining != 0);
        }
        return SCRIPT_VM_CONTINUE;
    }

    case BATTLE_VM_PAUSE_OBJECT_SCRIPT:
        object_script_state = BattleScriptState_GetByObjectId(
            (u16)command->arguments[0]);
        object_script_state->flags =
            (object_script_state->flags & ~BATTLE_AI_STATE_FLAG_DISABLED) |
            BATTLE_AI_STATE_FLAG_DISABLED;
        return BATTLE_AI_VM_YIELD;

    case BATTLE_VM_PAUSE_OBJECT_SCRIPTS_BY_OWNER: {
        s16 remaining;

        if (command->arguments[0] < 1) {
            command->arguments[0] = ai_state->owner_id;
        }
        script_selector = command->arguments[0];
        object_script_state = (BattleAIState *)(
            gBattleContext + BATTLE_VM_OBJECT_SCRIPT_STATES_OFFSET);
        if (script_selector < BATTLE_AI_TASK_OBJECT) {
            remaining = BATTLE_VM_OBJECT_SCRIPT_STATE_COUNT;
            do {
                if (command->arguments[0] ==
                        object_script_state->order_value &&
                    object_script_state->script != 0) {
                    object_script_state->flags =
                        (object_script_state->flags &
                         ~BATTLE_AI_STATE_FLAG_DISABLED) |
                        BATTLE_AI_STATE_FLAG_DISABLED;
                }
                remaining = (s16)(remaining - 1);
                object_script_state++;
            } while (remaining > 0);
        } else {
            u32 object_id = script_selector & BATTLE_AI_TASK_ACTOR_ID_MASK;

            remaining = BATTLE_VM_OBJECT_SCRIPT_STATE_COUNT;
            do {
                if (object_id == object_script_state->order_tie_break &&
                    object_script_state->script != 0) {
                    object_script_state->flags =
                        (object_script_state->flags &
                         ~BATTLE_AI_STATE_FLAG_DISABLED) |
                        BATTLE_AI_STATE_FLAG_DISABLED;
                    break;
                }
                remaining = (s16)(remaining - 1);
                object_script_state++;
            } while (remaining != 0);
        }
        return SCRIPT_VM_CONTINUE;
    }

    case BATTLE_VM_RESUME_OBJECT_SCRIPT:
        object_script_state = BattleScriptState_GetByObjectId(
            (u16)command->arguments[0]);
        object_script_state->flags &= ~BATTLE_AI_STATE_FLAG_DISABLED;
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_RESUME_OBJECT_SCRIPTS_BY_OWNER: {
        s16 remaining;

        if (command->arguments[0] < 1) {
            command->arguments[0] = ai_state->owner_id;
        }
        script_selector = command->arguments[0];
        object_script_state = (BattleAIState *)(
            gBattleContext + BATTLE_VM_OBJECT_SCRIPT_STATES_OFFSET);
        if (script_selector < BATTLE_AI_TASK_OBJECT) {
            remaining = BATTLE_VM_OBJECT_SCRIPT_STATE_COUNT;
            do {
                if (command->arguments[0] ==
                        object_script_state->order_value &&
                    object_script_state->script != 0) {
                    object_script_state->flags &=
                        ~BATTLE_AI_STATE_FLAG_DISABLED;
                }
                remaining = (s16)(remaining - 1);
                object_script_state++;
            } while (remaining > 0);
        } else {
            u32 object_id = script_selector & BATTLE_AI_TASK_ACTOR_ID_MASK;

            remaining = BATTLE_VM_OBJECT_SCRIPT_STATE_COUNT;
            do {
                if (object_id == object_script_state->order_tie_break &&
                    object_script_state->script != 0) {
                    object_script_state->flags &=
                        ~BATTLE_AI_STATE_FLAG_DISABLED;
                    break;
                }
                remaining = (s16)(remaining - 1);
                object_script_state++;
            } while (remaining != 0);
        }
        return SCRIPT_VM_CONTINUE;
    }

    case BATTLE_VM_BRANCH_COMPARE:
        comparison = 0;
        switch ((u16)command->arguments[0]) {
        case 0:
            if (command->arguments[1] == command->arguments[2]) {
                comparison = 1;
            }
            break;
        case 1:
            if (command->arguments[1] != command->arguments[2]) {
                comparison = 1;
            }
            break;
        case 2:
            if (command->arguments[1] < command->arguments[2]) {
                comparison = 1;
            }
            break;
        case 3:
            if (command->arguments[1] > command->arguments[2]) {
                comparison = 1;
            }
            break;
        case 4:
            if (command->arguments[1] <= command->arguments[2]) {
                comparison = 1;
            }
            break;
        case 5:
            if (command->arguments[1] >= command->arguments[2]) {
                comparison = 1;
            }
            break;
        case 6:
            comparison = command->arguments[1] & command->arguments[2];
            break;
        case 7:
            comparison = command->arguments[1] | command->arguments[2];
            break;
        case 8:
            comparison = command->arguments[1] ^ command->arguments[2];
            break;
        case 9:
            if (command->arguments[1] == 0) {
                comparison = 1;
            }
            break;
        case 10:
            comparison = ~command->arguments[1];
            break;
        }
        if ((u16)command->arguments[4] ==
            (comparison == command->arguments[3])) {
            state->script =
                (const u16 *)state->script + command->arguments[5];
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_BRANCH_OBJECT_PROPERTY:
        if ((u16)command->arguments[2] ==
            (BattleScript_GetProperty(
                 (u16)command->arguments[0], command->arguments[1]) > 0)) {
            state->script =
                (const u16 *)state->script + command->arguments[3];
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_BRANCH_ACTOR_HIT_LOCKED:
        if ((u16)command->arguments[1] ==
            BattleActor_IsHitLocked((u16)command->arguments[0])) {
            state->script =
                (const u16 *)state->script + command->arguments[2];
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_LEGACY_NOOP_0E0:
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_BRANCH_SCRIPT_ACTIVE:
        object_script_state = BattleAI_GetStateById(
            (u16)command->arguments[0]);
        if ((u16)command->arguments[1] ==
            (object_script_state->script != 0)) {
            state->script =
                (const u16 *)state->script + command->arguments[2];
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_BRANCH_ANIMATION_ACTIVE:
        if ((u16)command->arguments[2] ==
            BattleSceneObject_IsAnimationActiveById(
                (u16)command->arguments[0],
                (u16)command->arguments[1])) {
            state->script =
                (const u16 *)state->script + command->arguments[3];
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_PLAY_SOUND_WITH_HANDLE:
        sound_task_id = BattleSound_Play(
            (u16)command->arguments[0], (s16)command->arguments[1],
            (s16)command->arguments[2], 0);
        if (sound_task_id != -1) {
            BattleVm_WriteResult(vm, state, command, sound_task_id);
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_PLAY_SOUND:
        BattleSound_Play(
            (u16)command->arguments[0], (s16)command->arguments[1], 0, 0);
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_STOP_SOUND_TASK:
        if (command->arguments[0] != -1) {
            BattleSound_Stop((u16)command->arguments[0]);
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_STOP_ALL_SOUND_TASKS:
        for (index = 0; index < BATTLE_VM_SOUND_TASK_COUNT; index++) {
            if (((void **)(gBattleContext +
                           BATTLE_VM_SOUND_TASK_SLOTS_OFFSET))[index] != 0) {
                BattleSound_Stop(index);
            }
        }
        return SCRIPT_VM_CONTINUE;

    case BATTLE_VM_WAIT_SOUND_TASK:
        sound_task_id = command->arguments[0];
        if (sound_task_id == -1) {
            return SCRIPT_VM_CONTINUE;
        }
        if (((void **)(gBattleContext +
                       BATTLE_VM_SOUND_TASK_SLOTS_OFFSET))[
                (u16)sound_task_id] == 0) {
            return SCRIPT_VM_CONTINUE;
        }
        return BattleVm_RetryCurrentCommand(
            vm, state, BATTLE_VM_WAIT_SOUND_TASK
        );

    case BATTLE_VM_SPAWN_RASTER_PARTICLE:
        if ((command->argument_modes & (1 << 2)) == 0) {
            command->arguments[2] =
                (s32)(((u32)command->arguments[2] & 0xFFFF) |
                      ((u32)command->arguments[3] << 16)) /
                16;
        }
        BattleEffectParticle_Spawn(
            command->arguments[0], command->arguments[1],
            command->arguments[2]);
        return SCRIPT_VM_CONTINUE;

    default:
        return BattleVm_DispatchCommonOpcode(vm, state, command);
    }

    return SCRIPT_VM_CONTINUE;
}
