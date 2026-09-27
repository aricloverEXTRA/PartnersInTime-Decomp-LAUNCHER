#ifndef PIT_GAME_BATTLE_SCENE_H
#define PIT_GAME_BATTLE_SCENE_H

#include <game/battle_object.h>
#include <nitro.h>
#include <nitro/fx_mtx.h>
#include <game/sprite_output.h>
#include <game/graphics_resource.h>
#include <game/texture_allocation.h>

typedef struct BattleSceneObject BattleSceneObject;
typedef struct BattleMotionChannel BattleMotionChannel;
typedef struct BattleModel BattleModel;

typedef struct ModelRenderSortKey {
    u16 unknown_00[4];
    u16 unknown_08 : 14, mode : 2;
} ModelRenderSortKey;
typedef char ModelRenderSortKey_SizeCheck[sizeof(ModelRenderSortKey) == 10 ? 1 : -1];


typedef struct BattleModelVTable BattleModelVTable;
typedef struct GamePaletteEffectController GamePaletteEffectController;

typedef struct ModelRenderDescriptor {
    u8 unknown_00[0x54];
    union {
        u32 flags;
        struct { u32 overlap_priority : 2, unknown_02_31 : 30; } flag_bits;
    };
    u8 unknown_58, unknown_59;
    u16 unknown_5a;
    void *unknown_5c;
} ModelRenderDescriptor;
typedef char ModelRenderDescriptor_SizeCheck[sizeof(ModelRenderDescriptor) == 0x60 ? 1 : -1];


typedef struct BattleModelAnimationData BattleModelAnimationData;
typedef struct BattleSceneRenderOverride BattleSceneRenderOverride;
struct BattleSpriteTransform;
struct GameSpritePalette;
typedef void (*BattleMotionCallback)(BattleSceneObject *object,
                                     BattleMotionChannel *channel);

enum BattleModelFlag {
    BATTLE_MODEL_FLAG_02 = 1 << 2,
    BATTLE_MODEL_FLAG_ANIMATION_ACTIVE = 1 << 8,
    BATTLE_MODEL_FLAG_09 = 1 << 9,
    BATTLE_MODEL_FLAG_10_SHIFT = 10,
    BATTLE_MODEL_FLAG_11_SHIFT = 11,
    BATTLE_MODEL_FLAG_10 = 1 << BATTLE_MODEL_FLAG_10_SHIFT,
    BATTLE_MODEL_FLAG_11 = 1 << BATTLE_MODEL_FLAG_11_SHIFT,
    BATTLE_MODEL_ANIMATION_MODE_MASK = 0xF000,
    BATTLE_MODEL_ANIMATION_MODE_STATUS = 0x1000,
    BATTLE_MODEL_ANIMATION_MODE_BATTLE = 0x3000
};

enum {
    BATTLE_MOTION_CHANNEL_COUNT = 4
};

/* One row of the model's animation table, indexed by animation id. */
struct BattleModelAnimationData {
    u16 start_frame;
    u16 end_frame;
    u32 unknown_04;
};

typedef struct BattleModelControllerWork {
    void *controller;
    const s16 *primary_components[8];
    s16 primary_steps_q8[8];
    u8 primary_state, primary_mode;
    u8 unknown_36[2];
    const s16 *primary_track;
    const s16 *layer_components[8];
    s16 layer_steps_q8[8];
    u8 layer_states[8], layer_modes[8];
    const s16 *layer_tracks[8];
    u8 layer_animation_ids[8];
    s16 initial_animation_id;
    u16 unknown_a6;
} BattleModelControllerWork;

typedef char BattleModelControllerWork_SizeCheck[
    sizeof(BattleModelControllerWork) == 0xA8 ? 1 : -1];

/* One row of the model's frame table: the second halfword carries a nine-bit
   offset the renderer compares against the anchor after scaling it by 256. */
typedef struct BattleModelFrameEntry {
    u16 unknown_00;
    union {
        u16 raw;
        struct { u16 offset : 9, unknown_09_15 : 7; } bits;
    } value;
} BattleModelFrameEntry;

struct BattleSceneRenderOverride {
    int (*render)(BattleSceneObject *object, int pass);
};

typedef struct BattleVerticalMotionParameters {
    s32 quadratic_term;
    s32 linear_term;
    s16 start_z;
    s16 target_z;
} BattleVerticalMotionParameters;

typedef char BattleVerticalMotionParameters_SizeCheck[
    sizeof(BattleVerticalMotionParameters) == 0x0C ? 1 : -1];

typedef union BattleSceneFlags {
    u32 raw;
    s8 shadow_delay;
    s8 pending_state;
    struct {
        u32 state : 8;
        u32 shadow_alpha : 6;
        u32 use_alternate_model : 1;
        u32 unk_15 : 1;
        u32 suppress_shadow : 1;
        u32 use_raw_position : 1;
        u32 independent_flag : 1;
        u32 stop_impact_particles : 1;
        u32 unk_20_23 : 4;
        u32 render_mode : 3;
        u32 unk_27_31 : 5;
    } bits;
} BattleSceneFlags;

struct BattleMotionChannel {
    BattleMotionCallback callback;
    s32 elapsed_q8;
    s16 duration;
    u16 has_deferred_delta;
    s16 deferred_delta_x;
    s16 deferred_delta_y;
    s16 deferred_delta_z;
    s16 frame_delta_x;
    s16 frame_delta_y;
    s16 frame_delta_z;
    s16 parameters[8];
};

struct BattleModelVTable {
    void (*prepare_render)(BattleModel *model);
    void (*unknown_004)(BattleModel *model);
    void (*draw)(BattleModel *model, void *buffer, u8 *object_count,
                 u8 *affine_count);
    u8 unknown_00c[8];
    void (*unknown_014)(BattleModel *model);
    u8 unknown_018[0x0C];
    void (*start)(BattleModel *model);
    void (*stop)(BattleModel *model);
    u8 unknown_02c[8];
    int (*set_animation)(BattleModel *model, u8 animation_id, int argument_2);
    u8 unknown_038[4];
    int (*get_animation_id)(BattleModel *model);
    u8 unknown_040[4];
    u32 (*get_sort_key)(BattleModel *model);
    void (*unknown_048)(BattleModel *model, int enabled);
    int (*is_texture_dirty)(BattleModel *model);
    u8 unknown_050[0x18];
    int (*set_primary_animation)(BattleModel *model, u8 animation_id,
                                 int argument_2, int enabled);
    u8 unknown_06c[0x1C];
    int (*configure_animation_layer)(BattleModel *model, s8 layer,
                                     int animation_id, int enabled);
    u8 unknown_08c[4];
    int (*test_relation)(BattleModel *model, BattleModel *other);
    u8 unknown_094[8];
    void *(*get_palette_buffer)(BattleModel *model);
    void (*set_palette)(BattleModel *model, const void *source);
    const void *(*get_palette_source)(BattleModel *model);
    u8 unknown_0a8[0x20];
    struct BattleSpriteTransform *(*get_sprite_transform)(BattleModel *model);
};

#ifdef __cplusplus
struct BattleModel {
    virtual void prepare_render();
    virtual void unknown_04();
    virtual void draw(void *buffer, u8 *object_count, u8 *affine_count);
    virtual u32 get_object_size();
    virtual void unknown_10();
    virtual void unknown_14();
    virtual void unknown_18();
    virtual void unknown_1c();
    virtual void unknown_20();
    virtual void start();
    virtual void stop();
    virtual void unknown_2c();
    virtual void unknown_30();
    virtual int set_animation(u8 animation_id, int argument_2);
    virtual void unknown_38(BattleModel *source);
    virtual int get_animation_id();
    virtual int unknown_40();
    virtual u32 get_sort_key();
    virtual void unknown_48(int enabled);
    virtual int is_texture_dirty();
    virtual void set_palette_dirty(int enabled);
    virtual void unknown_54();
    virtual void unknown_58();
    virtual int restore_controller(const void *descriptor, void *controller, s16 animation);
    virtual void reset_controller_work();
    virtual int configure_controller(const void *descriptor, void *controller, s16 animation);
    virtual int set_primary_animation(u8 animation_id, int argument_2,
                                      int enabled);
    virtual void set_primary_state(u8 state);
    virtual void unknown_70();
    virtual void unknown_74();
    virtual void set_controller_animation(int animation, int reset);
    virtual void unknown_7c();
    virtual void unknown_80();
    virtual void unknown_84();
    virtual int configure_animation_layer(s8 layer, int animation_id,
                                           int enabled);
    virtual void unknown_8c();
    virtual int test_relation(BattleModel *other);
    virtual void unknown_94();
    virtual void unknown_98();
    virtual void *get_palette_buffer();
    virtual void set_palette(const void *source);
    virtual const void *get_palette_source();
    virtual void unknown_a8();
    virtual void unknown_ac();
    virtual void restore_resources(const void *descriptor);
    virtual void configure_resources(const void *descriptor);
    virtual void update_texture(const u32 *banks);
    virtual void unknown_bc();
    virtual void unknown_c0();
    virtual void unknown_c4();
    virtual struct BattleSpriteTransform *get_sprite_transform();
    BattleModel *render_previous;
    BattleModel *render_next;
    BattleSceneObject *owner;
    u8 screen;
    u8 unk_011[3];
    union {
        GameSpriteAllocation texture;
        struct { u8 unk_014[0x14]; u32 property_028; };
    };
    /* Render state shared with the owning object; bit 3 of byte 0x13 gates it. */
    union { u8 *property_02c; struct GameSpritePalette *palette; };
    union {
        u8 unk_030[8];
        struct { const void *resource_pixels; u32 unknown_034; };
    };
    /* Graphics resource header shared by frame and animation lookup. */
    union { const u16 *property_038; const GameGraphicsResource *resource; };
    const GameGraphicsObject *resource_objects;
    const GameGraphicsRange *resource_groups;
    const BattleModelFrameEntry *frames;
    BattleModelAnimationData *animation_data;
    const void *extra_resource_data;
    const u16 *texture_offsets;
    s16 animation_id;
    s16 property_056;
    u8 unk_058[2];
    union {
        u16 enemy_idle_frame;
        s16 effect_scale;
    };
    s16 animation_offset_x;
    s16 animation_offset_y;
    union { u8 unk_060[4]; struct { s16 palette_index, unknown_062; }; };
    s32 anchor_offset;
    s32 render_anchor_z;
    u8 unk_06c[8];
    s16 scale_x;
    s16 scale_y;
    u16 rotation_z;
    u16 unk_07a;
    union {
        u32 flags;
        struct { u32 unknown_00_21 : 22, alternate_resource : 1, unknown_23 : 1, unknown_24 : 1, unknown_25 : 1, unknown_26_31 : 6; } resource_flag_bits;
        struct {
            u32 unknown_00_01 : 2;
            u32 panel_animation_trigger : 1;
            u32 unknown_03 : 1;
            u32 render_linked : 1;
            u32 unknown_05_07 : 3;
            u32 animation_active : 1;
            u32 unknown_09 : 1;
            u32 facing_left : 1;
            u32 flip_y : 1;
            u32 animation_mode : 4;
            u32 texture_allocation_mode : 3;
            u32 unknown_19_21 : 3;
            u32 no_sort_key : 1;
            u32 unknown_23_31 : 9;
        } flag_bits;
    };
    u8 unk_080[4];
    union {
        BattleModelControllerWork controller_work;
        struct {
            void *animation_controller;
            u8 unk_088[0x70];
            s8 animation_layer_states[16];
            u8 unk_108[0x20];
            s16 initial_animation_id;
            u16 unk_12a;
        };
    };
    u8 unk_12c[4];
    GameTextureAllocation render_texture;
    GameTexturePalette render_palette;
    u8 unknown_160[2];
    union {
        u16 animation_state;
        struct {
            u16 state : 5;
            u16 mode : 1;
            u16 unknown_06_15 : 10;
        } animation_state_bits;
    };
    u16 owner_render_state;
    u8 unk_166[6];
    u16 unknown_16c, unknown_16e;
    u32 unknown_170;
    MtxFx44 transform;
    u8 unk_1b4[4];
};
#else
struct BattleModel {
    BattleModelVTable *vtable;
    BattleModel *render_previous;
    BattleModel *render_next;
    BattleSceneObject *owner;
    u8 screen;
    u8 unk_011[3];
    union {
        GameSpriteAllocation texture;
        struct { u8 unk_014[0x14]; u32 property_028; };
    };
    /* Render state shared with the owning object; bit 3 of byte 0x13 gates it. */
    union { u8 *property_02c; struct GameSpritePalette *palette; };
    union {
        u8 unk_030[8];
        struct { const void *resource_pixels; u32 unknown_034; };
    };
    /* Graphics resource header shared by frame and animation lookup. */
    union { const u16 *property_038; const GameGraphicsResource *resource; };
    const GameGraphicsObject *resource_objects;
    const GameGraphicsRange *resource_groups;
    const BattleModelFrameEntry *frames;
    BattleModelAnimationData *animation_data;
    const void *extra_resource_data;
    const u16 *texture_offsets;
    s16 animation_id;
    s16 property_056;
    u8 unk_058[2];
    union {
        u16 enemy_idle_frame;
        s16 effect_scale;
    };
    s16 animation_offset_x;
    s16 animation_offset_y;
    union { u8 unk_060[4]; struct { s16 palette_index, unknown_062; }; };
    s32 anchor_offset;
    s32 render_anchor_z;
    u8 unk_06c[8];
    s16 scale_x;
    s16 scale_y;
    u16 rotation_z;
    u16 unk_07a;
    union {
        u32 flags;
        struct { u32 unknown_00_21 : 22, alternate_resource : 1, unknown_23 : 1, unknown_24 : 1, unknown_25 : 1, unknown_26_31 : 6; } resource_flag_bits;
        struct {
            u32 unknown_00_01 : 2;
            u32 panel_animation_trigger : 1;
            u32 unknown_03 : 1;
            u32 render_linked : 1;
            u32 unknown_05_07 : 3;
            u32 animation_active : 1;
            u32 unknown_09 : 1;
            u32 facing_left : 1;
            u32 flip_y : 1;
            u32 animation_mode : 4;
            u32 texture_allocation_mode : 3;
            u32 unknown_19_21 : 3;
            u32 no_sort_key : 1;
            u32 unknown_23_31 : 9;
        } flag_bits;
    };
    u8 unk_080[4];
    union {
        BattleModelControllerWork controller_work;
        struct {
            void *animation_controller;
            u8 unk_088[0x70];
            s8 animation_layer_states[16];
            u8 unk_108[0x20];
            s16 initial_animation_id;
            u16 unk_12a;
        };
    };
    u8 unk_12c[4];
    GameTextureAllocation render_texture;
    GameTexturePalette render_palette;
    u8 unknown_160[2];
    union {
        u16 animation_state;
        struct {
            u16 state : 5;
            u16 mode : 1;
            u16 unknown_06_15 : 10;
        } animation_state_bits;
    };
    u16 owner_render_state;
    u8 unk_166[6];
    u16 unknown_16c, unknown_16e;
    u32 unknown_170;
    MtxFx44 transform;
    u8 unk_1b4[4];
};
#endif

struct BattleSceneObject {
    BattleSceneObject *motion_next;
    s16 x;
    s16 y;
    s16 z;
    s16 motion_origin_x;
    s16 motion_origin_y;
    s16 motion_origin_z;
    s16 motion_target_x;
    s16 motion_target_y;
    s16 motion_target_z;
    s16 time_step_adjustment_q8;
    s16 smoothed_travel_distance;
    u16 time_paused;
    BattleMotionChannel motion_channels[BATTLE_MOTION_CHANNEL_COUNT];
    BattleSceneResource *resource;
    BattleModel *primary_model;
    BattleModel *alternate_model;
    BattleSceneRenderOverride *render_override;
    u8 unk_0cc[0x14];
    s32 loaded_resource_id;
    s16 animation_id;
    s16 previous_base_animation_id;
    s16 animation_variant_offset;
    s16 effect_anchor_z;
    u16 actor_id;
    u16 linked_actor_id;
    u16 render_state;
    u8 unk_0f2[2];
    BattleSceneFlags flags;
    s16 property_0f8;
    s16 property_0fa;
    s8 property_0fc;
    s8 property_0fd;
    s8 property_0fe;
    s8 property_0ff;
    s8 property_100;
    s8 property_101;
    union {
        s8 property_102;
        u8 removal_state_102;
    };
    union {
        s8 property_103;
        u8 removal_state_103;
    };
};

typedef char BattleMotionChannel_SizeCheck[
    sizeof(BattleMotionChannel) == 0x28 ? 1 : -1];
typedef char BattleSceneObject_SizeCheck[
    sizeof(BattleSceneObject) == 0x104 ? 1 : -1];
typedef char BattleModel_SizeCheck[sizeof(BattleModel) == 0x1B8 ? 1 : -1];

extern BattleSceneObject *gBattleMotionObjectList;
/* One render list per display engine. */
extern BattleModel *gModelRenderList[2];
extern BattleModel *gModelRenderListTail[2];

#ifdef __cplusplus
extern "C" {
#endif

u32 BattleMath_StartSqrt(u32 value);
u32 BattleMath_WaitForSqrtResult(void);

void BattleSceneObject_SetStateFlags(BattleSceneObject *object, int state,
                                     int independent_flag);
void BattleScene_RenderModels(void);
/* The context argument is forwarded by the caller but not read by this stage. */
void BattleScene_RenderSubscreen(void *unused_context);
void BattleScene_RenderShadows(void);
void BattleScene_UpdateAlternateModelAnchor(BattleModel *model);
int BattleScene_TestModelRelation(BattleSceneObject *first,
                                  BattleSceneObject *second,
                                  int first_model, int second_model);
void BattleScene_DrawShadow(int variant, int animation_state,
                            int x, int y, int height, int alpha);
void BattleScene_HideQueuedObjects(void);
int BattleScene_UpdateShadowVisibility(BattleSceneObject *object);
void BattleSceneObject_SwapSlots(u32 first_id, u32 second_id);
void BattleSceneObject_SetModelFlag11ById(int object_id, int enabled);
void BattleSceneObject_SetModelFlag10(BattleSceneObject *object, int enabled);
void BattleSceneObject_SetModelFlag10ById(int object_id, int enabled);
BattleModel *BattleSceneObject_GetActiveModel(BattleSceneObject *object);
int BattleSceneObject_GetAttachedModelProperty(BattleSceneObject *object, int property, int element);
int BattleSceneObject_GetAuxModelProperty(BattleSceneObject *object, int property, int element, int unused);
void BattleSceneObject_SetAuxModelProperty(BattleSceneObject *object, int property, int value, int unused);
BattleModel *BattleSceneObject_GetActiveModelById(int object_id);
BattleMotionChannel *BattleSceneObject_GetMotionChannel(
    BattleSceneObject *object, int channel_index);
void BattleSceneObject_SnapshotPosition(BattleSceneObject *object);
void BattleSceneObject_MoveByImmediate(BattleSceneObject *object,
                                       int delta_x, int delta_y, int delta_z);
void BattleSceneObject_AddPositionDelta(BattleSceneObject *object,
                                        int delta_x, int delta_y, int delta_z);
void BattleSceneObject_MoveToObject(BattleSceneObject *, int, int, int, int, int, BattleSceneObject *);
void BattleSceneObject_UpdateMoveToObject(BattleSceneObject *, BattleMotionChannel *);

void BattleSceneObject_AdjustPosition(BattleSceneObject *object,
                                      int delta_x, int delta_y, int delta_z);
void BattleSceneObject_MoveTo(BattleSceneObject *object, int channel_index,
                              int target_x, int target_y, int target_z,
                              int duration);
void BattleSceneObject_UpdateMoveTo(BattleSceneObject *object,
                                    BattleMotionChannel *channel);
void BattleSceneObject_MoveBy(BattleSceneObject *object, int channel_index,
                              int delta_x, int delta_y, int delta_z,
                              int duration);
void BattleSceneObject_UpdateMoveBy(BattleSceneObject *object,
                                    BattleMotionChannel *channel);
void BattleSceneObject_UpdateTravelDistance(BattleSceneObject *object);
void BattleSceneMotion_UpdateAll(void);
void BattleSceneObject_ApplyMovement(u32 object_id, int channel_index,
                                     int movement_kind, int x, int y, int z,
                                     int duration, int target_object_id,
                                     int argument_8, int argument_9);
void BattleSceneObject_SetAnimation(BattleSceneObject *object,
                                    int animation_id, int argument_2);
u32 BattleSceneObject_SetAnimationFromComponent(BattleSceneObject *object,
                                                int animation_id,
                                                int argument_2,
                                                int component_index);
u32 BattleSceneObject_SetBattleAnimation(BattleSceneObject *object,
                                         int animation_id, int model_flag);
u32 BattleSceneObject_SetBattleAnimationById(u32 object_id,
                                             int animation_id,
                                             int model_flag);
int BattleSceneObject_StartAcceleratedMotionForDuration(
    BattleSceneObject *object, int channel_index,
    int direction_x, int direction_y, int direction_z,
    int distance, int duration, int terminal_velocity,
    int positive_acceleration
);
int BattleSceneObject_StartMotionWithAccelerationForDuration(
    BattleSceneObject *object, int channel_index,
    int direction_x, int direction_y, int direction_z,
    int distance, int duration, int acceleration);
int BattleSceneObject_StartMotionWithVelocityForDuration(
    BattleSceneObject *object, int channel_index,
    int direction_x, int direction_y, int direction_z,
    int distance, int duration, int velocity);
int BattleMotion_StartWithVelocityRatio(
    BattleSceneObject *object, int channel_index,
    int direction_x, int direction_y, int direction_z,
    int distance, int duration, int velocity_ratio_q8,
    int prefer_longer_duration);
int BattleMotion_StartBallistic(
    BattleSceneObject *object, int channel_index,
    int direction_x, int direction_y, int direction_z,
    int distance, int acceleration, int signed_extent,
    int prefer_longer_duration);
int BattleSceneObject_StartMotionWithPeakDistance(
    BattleSceneObject *object, int channel_index,
    int direction_x, int direction_y, int direction_z,
    int distance, int peak_distance, int velocity,
    int prefer_longer_duration);
int BattleSceneObject_StartScaledAcceleratedMotion(
    BattleSceneObject *object, int channel_index,
    int direction_x, int direction_y, int direction_z,
    int distance, int initial_velocity, int acceleration,
    int prefer_longer_duration);
int BattleSceneObject_StartAcceleratedMotion(
    BattleSceneObject *object, int channel_index,
    int direction_x, int direction_y, int direction_z,
    int distance, int initial_velocity, int acceleration,
    int prefer_longer_duration
);
void BattleSceneObject_UpdateAcceleratedMotion(
    BattleSceneObject *object, BattleMotionChannel *channel);
int BattleSceneObject_StartVerticalMotionTo(
    BattleSceneObject *object, int channel_index,
    int initial_velocity, int acceleration, int target_z,
    int prefer_longer_duration);
int BattleSceneObject_StartVerticalArc(
    BattleSceneObject *object, int channel_index,
    int duration_or_velocity, int curve_parameter, int target_z);
void BattleSceneObject_UpdateVerticalMotion(
    BattleSceneObject *object, BattleMotionChannel *channel);
s16 *BattleSceneObject_BeginMotionChannel(BattleSceneObject *object,
                                          int channel_index, int duration,
                                          BattleMotionCallback callback);
void BattleSceneObject_UnlinkMotion(BattleSceneObject *object);
void BattleSceneObject_StopMotionChannel(BattleSceneObject *object,
                                        int channel_index,
                                        int apply_deferred_delta);
int BattleSceneObject_IsAnimationChannelActive(BattleSceneObject *object,
                                               int channel_index);
int BattleSceneObject_IsAnimationActiveById(u32 object_id,
                                            int channel_index);
int BattleSceneObject_ConfigureAnimationLayer(int object_id, int layer);
BattleSceneObject *BattleSceneObject_GetById(u32 object_id);
void BattleScene_SetAuxiliaryObjectsPaused(int paused);
/* Reuses a matching slot or reserves aligned state storage in the resource arena. */
BattleSceneRenderOverride *BattleSceneObject_ReserveRenderOverride(
    BattleSceneObject *object, u32 state_size);
void BattleModel_StopPrimary(BattleModel *model);
void BattleModel_StopAlternate(BattleModel *model);
int BattleModel_StartAnimation(BattleModel *model, int animation_id, int argument);
int BattleModel_SetAlpha(BattleModel *model, u8 alpha, u8 mode);
void BattleModel_AppendRenderList(BattleModel *model);
void BattleModel_UnlinkRenderList(BattleModel *model);
void BattleModel_DetachRenderList(BattleModel *model);

int BattleModel_ReleaseTexture(BattleModel *model, int force);
void BattleModel_RestoreRenderList(BattleModel *model);
void BattleModel_UnlinkTexture(BattleModel *model);
BattleModel *BattleModel_DestroyResourceBase(BattleModel *model);
BattleModel *BattleModel_DeleteResource(BattleModel *model);
BattleModel *BattleModel_DestroyResource(BattleModel *model);
BattleModel *BattleRenderModel_Delete(BattleModel *model);
BattleModel *BattleRenderModel_Destroy(BattleModel *model);
BattleModel *BattleRenderModel_Init(BattleModel *model);
void BattleModelController_Reset(BattleModel *model);
void BattleModelController_SetLayerAnimation(BattleModel *model, int layer, int animation, int reset);
void BattleModelController_SetLayerSpeed(BattleModel *model, int layer, s16 speed_q8, int use_model_speed);
void BattleModelController_SetLayerMode(BattleModel *model, int layer, u8 mode);
void BattleModelController_SetLayerState(BattleModel *model, int layer, u8 state);
void BattleModelController_SetPrimarySpeed(BattleModel *model, int component, s16 speed_q8, int use_model_speed);
void BattleModelController_SetPrimaryMode(BattleModel *model, u8 mode);
void BattleModelController_SetPrimaryState(BattleModel *model, u8 state);
void BattleModelController_SetPrimaryAnimation(BattleModel *model, u8 animation, int frame, int update_controller);
u32 BattleModelController_GetObjectSize(BattleModel *model);
int BattleModelController_Configure(BattleModel *model, const void *descriptor, void *controller, s16 animation);
int BattleModelController_Restore(BattleModel *model, const void *descriptor, void *controller, s16 animation);
void BattleModelController_ConfigureResources(BattleModel *model, const void *descriptor);
int BattleModelController_RestoreResources(BattleModel *model, const void *descriptor);
BattleModel *BattleModelController_DestroyBase(BattleModel *model);
BattleModel *BattleModelController_Delete(BattleModel *model);
BattleModel *BattleModelController_Destroy(BattleModel *model);
BattleModel *BattleModelController_InitBase(BattleModel *model);
BattleModel *BattleModelController_Init(BattleModel *model);
int BattleModel_GetTextureConversionSize(int boundary, int alternate, const GameGraphicsResource *resource);
int BattleModel_GetScreenTextureConversionSize(int screen, int alternate, const GameGraphicsResource *resource);
BattleModel *BattleModel_InitResourceState(BattleModel *model);
/* The original callers pass a mode although this base initializer ignores it. */
void BattleModel_InitDescriptor(void *descriptor, int unused_mode);
u16 BattleModel_ReadSortKeyLow(const u16 *key, int unused);
u16 BattleModel_ReadSortKeyHigh(const u16 *key, int unused);
u16 BattleModel_ReadSortKeyOverride(const u16 *unused, int index);
void BattleModel_SetSortKeyOverride(const u16 *table);
u16 BattleModel_GetRelativeAnimationId(BattleModel *model);
u16 BattleModel_GetRelativeAnimationCount(BattleModel *model);
void BattleModel_SetInitialAnimation(BattleModel *model, s16 animation);
void *BattleModel_GetPaletteBuffer(BattleModel *model);
void BattleModel_SetPalette(BattleModel *model, const void *source);
const void *BattleModel_GetPaletteSource(BattleModel *model);
int BattleModel_GetPaletteColorCount(BattleModel *model);
void BattleModel_SetPaletteBuffered(BattleModel *model, int enabled);
void BattleModel_SetPaletteMask(BattleModel *model, GamePaletteEffectController *table, int index, int enabled);
void BattleModel_UpdateSpriteTexture(BattleModel *model, int unused_mode);
void BattleModel_CopyAnimationLayers(BattleModel *source, BattleModel *destination);
void BattleModel_RestoreSourcePalette(BattleModel *model);

void BattleRenderModel_InitDescriptor(ModelRenderDescriptor *descriptor, int mode);

u8 BattleRenderModel_ReadSortKeyMode(BattleModel *unused, const ModelRenderSortKey *key);
void BattleRenderModel_SetPaletteBuffered(BattleModel *model, int enabled);
int BattleRenderModel_GetPaletteColorCount(BattleModel *model);
const void *BattleRenderModel_GetPaletteSource(BattleModel *model);
void BattleRenderModel_SetPalette(BattleModel *model, const void *source);
void *BattleRenderModel_GetPaletteBuffer(BattleModel *model);
MtxFx44 *BattleRenderModel_GetTransform(BattleModel *model);
int BattleRenderModel_ConfigureController(BattleModel *model, const void *descriptor, void *controller, s16 animation);
void BattleRenderModel_UnlinkTexture(BattleModel *model);
void BattleRenderModels_UpdateTextures(const u32 *banks, u8 filter);
int BattleRenderModel_GetTextureConversionSize(const GameGraphicsResource *resource, u32 alternate);
int BattleRenderModel_RestoreController(BattleModel *model, const void *descriptor, void *controller, s16 animation);
void BattleRenderModel_ReleaseResources(BattleModel *model);
void BattleRenderModel_UpdateTexture(BattleModel *model, const u32 *banks);
void BattleRenderModel_NoOp(BattleModel *model);
u32 BattleRenderModel_GetObjectSize(BattleModel *model);
void BattleRenderModel_SetTextureDirty(BattleModel *model, int enabled);
int BattleRenderModel_IsTextureDirty(BattleModel *model);
void BattleRenderModel_SetPaletteDirty(BattleModel *model, int enabled);
int BattleRenderModel_IsPaletteDirty(BattleModel *model);
void GameTexture_ResetLists(void);

#ifdef __cplusplus
}
#endif

#endif
