#ifndef PARTNERS_IN_TIME_GAME_FIELD_ENTITY_H
#define PARTNERS_IN_TIME_GAME_FIELD_ENTITY_H

#include <nitro.h>
#include <nitro/fx.h>
#include <game/sprite_output.h>

enum FieldEntityStateFlag {
    FIELD_ENTITY_STATE_ACTIVE = 1 << 1,
    FIELD_ENTITY_STATE_FLAG_02 = 1 << 2
};

enum FieldRuntimeEntityFlag {
    FIELD_ENTITY_RETAIN_OFFSCREEN_CONTACT = 1 << 8,
    FIELD_ENTITY_RESERVED_STATE = 1 << 12,
    FIELD_ENTITY_CONTACT_MODE_MASK = 7,
    FIELD_ENTITY_TURN_TO_INTERACTOR = 1 << 11,
    FIELD_ENTITY_TRACK_GROUND = 1 << 12,
    FIELD_ENTITY_IGNORE_NAVIGATION_OBSTACLE = 1 << 13,
    FIELD_ENTITY_BODY_COLLISION_ENABLED = 1 << 14,
    FIELD_ENTITY_SHADOW_ENABLED = 1 << 15,
    FIELD_ENTITY_SHADOW_SUPPORT_ENABLED = 1 << 17,
    FIELD_ENTITY_SHADOW_STYLE_SHIFT = 18,
    FIELD_ENTITY_SHADOW_STYLE_MASK = 7 << FIELD_ENTITY_SHADOW_STYLE_SHIFT,
    FIELD_ENTITY_SAVED_RESOURCE = 1 << 16,
    FIELD_ENTITY_SAVED_PALETTE_PROFILE = 1 << 17,
    FIELD_ENTITY_SAVED_RESOURCE_ANIMATION = 1 << 18,
    FIELD_ENTITY_SAVED_ANIMATION = 1 << 19,
    FIELD_ENTITY_SAVED_BEHAVIOR = 1 << 13,
    FIELD_ENTITY_SYNC_HORIZONTAL = 1 << 0,
    FIELD_ENTITY_SYNC_VERTICAL = 1 << 1,
    FIELD_ENTITY_ALTERNATE_COLLISION_FACES = 1 << 3,
    FIELD_ENTITY_HORIZONTAL_SYNC_DIRTY = 1 << 4,
    FIELD_ENTITY_VERTICAL_SYNC_DIRTY = 1 << 5,
    FIELD_ENTITY_AUTO_AUXILIARY_PRIORITY = 1 << 25,
    FIELD_ENTITY_AUTO_PRIORITY_0 = 1 << 26,
    FIELD_ENTITY_AUTO_PRIORITY_1 = 1 << 27,
    FIELD_ENTITY_AUTO_PRIORITY_2 = 1 << 28,
    FIELD_ENTITY_AUTO_PRIORITY_3 = 1 << 29
};

enum FieldRenderObjectFlag {
    FIELD_RENDER_SEMITRANSPARENT = 1 << 3,
    FIELD_RENDER_ANIMATION_ACTIVE = 1 << 8,
    FIELD_RENDER_ANIMATION_SUPPRESSED = 1 << 9
};

typedef struct FieldEntity FieldEntity;
typedef struct FieldRenderSnapshot FieldRenderSnapshot;
typedef struct FieldEntityVTable FieldEntityVTable;
typedef struct FieldRenderObject FieldRenderObject;
typedef struct FieldRenderObjectVTable FieldRenderObjectVTable;
typedef struct FieldRuntimeEntity FieldRuntimeEntity;
typedef struct FieldNavigationSurface FieldNavigationSurface;
typedef void (*FieldEntityVisibilityCallback)(FieldEntity *entity);

typedef struct FieldContactDirectionFlags {
    u32 enabled_mask : 6;
    u32 reserved : 26;
} FieldContactDirectionFlags;

typedef struct FieldBaseStateFlags {
    /* Enables waiting for the entity renderer's current animation. */
    u32 animation_wait_enabled : 1;
    u32 unknown_01 : 1;
    u32 facing_direction : 3;
    u32 previous_facing_direction : 3;
    u32 retain_offscreen_contact : 1;
    u32 unknown_09 : 1, outside_screen : 1, previous_outside_screen : 1;
    u32 reserved_state : 1;
    u32 blink_mode : 2;
    u32 blink_paused : 1;
    u32 blink_phase : 1;
    s32 blink_frames : 8;
    u32 blink_offset : 7;
} FieldBaseStateFlags;

/* Shared linear controller: active bits select X, Y and Z independently.
 * The planar implementation uses X/Y; spatial overrides share this layout. */
typedef struct FieldLinearController {
    union {
        u32 flags;
        struct {
            u32 active_axes : 3, paused : 1, fixed_duration : 1;
            u32 stop_at_destination : 1, braking : 1;
            u32 stop_mask_a : 6, stop_mask_b : 6, unknown_19_31 : 13;
        } bits;
        struct { u32 x : 1, y : 1, z : 1, unknown_03_31 : 29; } axes;
    };
    u16 direction, vertical_direction;
    u32 elapsed_frames;
    union { u32 duration_frames; fx32 speed; } timing;
    fx32 acceleration, deceleration, maximum_speed;
    fx32 start_x, start_y, start_z;
    fx32 destination_x, destination_y, destination_z;
    fx32 velocity_x, velocity_y, velocity_z;
    FieldRuntimeEntity *target;
} FieldLinearController;

typedef char FieldLinearController_SizeCheck[sizeof(FieldLinearController) == 0x44 ? 1 : -1];

/* Angles use 65536 units per turn. Planar orbit zero points upward.
 * The spatial implementation uses the same controller at +0x238. */
typedef struct FieldOrbitController {
    union {
        u32 flags;
        struct {
            u32 active : 1, paused : 1, fixed_duration : 1;
            u32 plane : 2; /* 0: Y/Z, 1: X/Z, 2: X/Y. */
            u32 stop_at_destination : 1, braking : 1;
            u32 stop_mask_a : 6, stop_mask_b : 6, unknown_19_23 : 5;
            s32 direction : 8;
        } bits;
        struct { u8 unknown_00[3]; s8 direction; } bytes;
    };
    u32 elapsed_frames;
    fx32 speed;
    union { u32 duration_frames; fx32 acceleration; } timing;
    fx32 deceleration, maximum_speed;
    s32 initial_angle;
    s32 destination_angle, angle, remaining_angle;
    fx32 vertical_scale, radius, vertical_radius, circumference;
    fx32 center_x, center_y, center_z;
    FieldRuntimeEntity *center_entity;
} FieldOrbitController;

typedef char FieldOrbitController_SizeCheck[sizeof(FieldOrbitController) == 0x48 ? 1 : -1];

typedef struct FieldTransformFlags {
    u16 scaling_active : 2;
    u16 rotation_active : 1;
    u16 unknown_03_15 : 13;
} FieldTransformFlags;

/* Shared scale/rotation controller; angles use 65536 units per turn. */
typedef struct FieldTransformController {
    union {
        u16 flags;
        struct {
            u16 scale_x_active : 1, scale_y_active : 1, rotation_active : 1;
            u16 scaling_paused : 1, rotation_paused : 1;
            u16 scaling_fixed_duration : 1, rotation_fixed_duration : 1;
            u16 stop_rotation_at_destination : 1, unknown_08_15 : 8;
        } bits;
        struct { u16 scaling_active : 2, unknown_02_15 : 14; } scaling;
        struct { u16 active : 3, unknown_03_15 : 13; } state;
    };
    union { u16 scale_x_duration; s16 scale_x_speed; } x;
    union { u16 scale_y_duration; s16 scale_y_speed; } y;
    u16 rotation_duration;
    s32 rotation_speed;
    s16 scale_x_step, scale_y_step;
    s32 rotation_step, remaining_rotation;
    s16 start_scale_x, start_scale_y;
    s32 start_rotation;
    s16 destination_scale_x, destination_scale_y;
    s32 destination_rotation;
    u32 scaling_elapsed, rotation_elapsed;
} FieldTransformController;
typedef char FieldTransformController_SizeCheck[sizeof(FieldTransformController) == 48 ? 1 : -1];

typedef struct FieldRenderSortKey {
    u32 order : 28;
    u32 layer : 4;
} FieldRenderSortKey;

typedef struct FieldEntityRuntimeFlags {
    u32 sync_horizontal : 1;
    u32 sync_vertical : 1;
    u32 unknown_02 : 1;
    u32 alternate_collision_faces : 1;
    u32 horizontal_sync_dirty : 1;
    u32 vertical_sync_dirty : 1;
    u32 unknown_06 : 1, unknown_07 : 1, unknown_08 : 1;
    u32 contact_mask_b : 6, previous_contact_mask_b : 6;
    u32 unknown_21 : 1, unknown_22 : 1, unknown_23_24 : 2;
    u32 auto_auxiliary_priority : 1;
    u32 auto_priority_0 : 1;
    u32 auto_priority_1 : 1;
    u32 auto_priority_2 : 1;
    u32 auto_priority_3 : 1;
    u32 unknown_30_31 : 2;
} FieldEntityRuntimeFlags;

typedef struct FieldEntityFieldStateFlags {
    u32 contact_mode : 3;
    u32 unknown_03 : 1;
    u32 vertical_motion_active : 1;
    u32 previous_vertical_motion_active : 1, vertical_motion_paused : 1, unknown_07 : 1, unknown_08_10 : 3;
    u32 turn_to_interactor : 1;
    u32 track_ground : 1;
    u32 ignore_navigation_obstacle : 1;
    u32 body_collision_enabled : 1;
    u32 shadow_enabled : 1;
    u32 unknown_16 : 1;
    u32 shadow_support_enabled : 1;
    u32 shadow_style : 3;
    u32 unknown_21_25 : 5, unknown_26_27 : 2, unknown_28_31 : 4;
} FieldEntityFieldStateFlags;

typedef struct FieldCollisionStateFlags {
    u32 unknown_00 : 1, unknown_01 : 1, unknown_02 : 1, unknown_03 : 1;
    u32 unknown_04 : 1, unknown_05 : 1, unknown_06 : 1, unknown_07 : 1;
    u32 unknown_08_15 : 8;
    u32 reserved_0 : 1;
    u32 reserved_1 : 1;
    u32 unknown_18_19 : 2, unknown_20_31 : 12;
} FieldCollisionStateFlags;

typedef struct FieldRenderStateFlags {
    u32 unknown_00 : 1, unknown_01 : 1, unknown_02 : 1;
    u32 semitransparent : 1;
    u32 render_linked : 1;
    u32 unknown_05_07 : 3;
    u32 animation_active : 1;
    u32 animation_suppressed : 1;
    u32 unknown_10 : 1, unknown_11 : 1;
    u32 behavior_state : 4;
    u32 unknown_16_31 : 16;
} FieldRenderStateFlags;

typedef struct FieldSavedPresentationFlags {
    u32 resource_set : 1;
    u32 palette_slot : 5;
    u32 unknown_06 : 1;
    u32 behavior_mode : 3;
    u32 saved_behavior_mode : 3;
    u32 has_saved_behavior : 1;
    u32 unknown_14 : 1, unknown_15 : 1;
    u32 has_saved_resource : 1;
    u32 has_saved_palette_profile : 1;
    u32 has_saved_resource_animation : 1;
    u32 has_saved_animation : 1;
    u32 unknown_20_29 : 10;
    u32 skip_auxiliary_renderer : 1;
    u32 unknown_31 : 1;
} FieldSavedPresentationFlags;

typedef struct FieldRoamingFlags {
    u32 clamp_to_boundary : 1;
    u32 unknown_01_07 : 7, unknown_08_13 : 6, unknown_14_19 : 6, unknown_20_31 : 12;
} FieldRoamingFlags;

typedef struct FieldInteractionFlags {
    u32 jump_first_strike_enabled : 1;
    u32 spiked_jump_response : 1;
    u32 remove_after_special_contact : 1;
    u32 remove_immediately_for_battle : 1;
    u32 unknown_04_13 : 10;
    s32 block_bounce_state : 8;
    u32 unknown_22_31 : 10;
} FieldInteractionFlags;

struct FieldRenderObjectVTable {
    u8 unknown_00[0x48];
    void (*unknown_48)(FieldRenderObject *, int);
    u8 unknown_4c[0x14];
    void (*reset_controller_work)(FieldRenderObject *render_object);
    u8 unknown_64[4];
    void (*set_animation)(FieldRenderObject *render_object,
                          u8 resource_animation, s16 animation_id,
                          int restart);
    u8 unknown_6c[0xC];
    void (*set_controller_animation)(FieldRenderObject *render_object, int animation, int reset);
    void (*set_palette_animation_paused)(
        FieldRenderObject *render_object, s8 slot, int paused);
    void (*set_palette_animation_mode)(
        FieldRenderObject *render_object, s8 slot, u8 mode);
    u8 unknown_84[4];
    void (*bind_palette_animation)(FieldRenderObject *render_object,
                                   s8 slot, s16 animation_id,
                                   int restart);
    void (*request_base_palette_reload)(FieldRenderObject *render_object);
};

struct FieldEntityVTable {
    u8 unknown_00[0x0C];
    void (*stop_script)(FieldEntity *entity);
    void (*pause_script)(FieldEntity *entity);
    void (*resume_script)(FieldEntity *entity);
    int (*get_property)(FieldEntity *entity, int property_id);
    u8 unknown_1c[0x10];
    void (*release_renderers)(FieldEntity *entity);
    u8 unknown_30[0x20];
    void (*cancel_linear_movement)(FieldEntity *entity, FieldLinearController *controller,
                                   int snap_to_destination);
    u8 unknown_54[8];
    void (*cancel_orbit_movement)(FieldEntity *entity, FieldOrbitController *controller,
                                     int snap_to_destination);
    void (*set_visible)(FieldEntity *entity, int visible);
    u8 unknown_64[0x10];
    void (*start_blink)(FieldEntity *entity, int mode, const s8 *durations, u8 length,
                        FieldEntityVisibilityCallback show, FieldEntityVisibilityCallback hide);
    u8 unknown_78[0x14];
    void (*set_collision_response_channels)(
        FieldEntity *entity, int channel_0, int channel_1, int channel_2,
        int channel_3, int channel_4);
    void (*set_collision_response_channels_masked)(
        FieldEntity *entity, u16 channel_mask, int enabled);
    void (*restore_collision_response_channels)(FieldEntity *entity);
};

typedef struct FieldEntity {
#ifdef __cplusplus
    virtual void unknown_00();
    virtual void unknown_04();
    virtual void unknown_08();
    virtual void stop_script();
    virtual void pause_script();
    virtual void resume_script();
    virtual int get_property(int property_id);
    virtual void update_bounds();
    virtual void unknown_20(const void *resource);
    virtual void reset_motion_parameters();
    virtual void unknown_28();
    virtual void release_renderers();
    virtual void unknown_30();
    virtual int is_outside_screen();
    virtual void update_locomotion_state();
    virtual void map_locomotion_state();
    virtual void unknown_40(FieldRuntimeEntity **entities);
    virtual void unknown_44();
    virtual void update_linear_movement(FieldLinearController *controller);
    virtual int check_linear_completion(FieldLinearController *controller);
    virtual void cancel_linear_movement(FieldLinearController *controller, int snap_to_destination);
    virtual void update_orbit_movement(FieldOrbitController *controller);
    virtual int advance_orbit_frame(FieldOrbitController *controller);
    virtual void cancel_orbit_movement(FieldOrbitController *controller, int snap_to_destination);
    virtual void set_visible(int visible);
    virtual void unknown_64();
    virtual void update_animation(int mode, int restart);
    virtual void update_screen_position(s16 camera_x, s16 camera_y);
    virtual void unknown_70();
    virtual void start_blink(int mode, const s8 *durations, u8 length,
                             FieldEntityVisibilityCallback show, FieldEntityVisibilityCallback hide);
    virtual void unknown_78();
    virtual void save_render_snapshot(FieldRenderSnapshot *snapshot);
    virtual void restore_render_snapshot(const FieldRenderSnapshot *snapshot);
    virtual void set_navigation_surfaces(FieldNavigationSurface *surfaces, const void *resource);
    virtual void unknown_88();
    virtual void set_collision_response_channels(
        int channel_0, int channel_1, int channel_2, int channel_3, int channel_4);
    virtual void set_collision_response_channels_masked(u16 channel_mask, int enabled);
    virtual void restore_collision_response_channels();
    virtual void unknown_98(void *argument);
    virtual void unknown_9c();
    /* The area updater forwards its priority field as a full register value. */
    virtual void update_overlap_priorities(int default_priority);
#else
    FieldEntityVTable *vtable;
#endif
    u8 index;
    union {
        u8 unknown_005[3];
        struct {
            u8 update_order;
            union { u8 unknown_006[2]; struct { u8 render_order, unknown_007; }; };
        };
    };
    union { u16 unknown_008; u16 script_suppression_variable; };
    union {
        u16 property_00a;
        struct {
            u16 property_00a_flag_00 : 1;
            u16 property_00a_unknown_01 : 1;
            u16 subtype : 4;
            u16 resource_set : 1;
            u16 property_00a_unknown_07_08 : 2;
            u16 heap : 3;
            u16 property_00a_unknown_12_14 : 3;
            u16 property_00a_unknown_15 : 1;
        } property_00a_bits;
        struct { u16 unknown_00_06 : 7, unknown_07 : 1, unknown_08 : 1, unknown_09_15 : 7; } visibility_bits;
    };
    union {
        struct {
            union { s16 unknown_00c; s16 startup_script; };
            u8 unknown_00e[0x12];
        };
        struct {
            u8 unknown_00c_prefix[4];
            FieldRuntimeEntity *update_previous, *update_next;
            union {
                u8 unknown_018[8];
                struct { FieldRuntimeEntity *render_previous, *render_next; };
            };
        };
    };
    union {
        u8 state_payload[0xCC];
        struct {
            u8 unknown_020[0xB0];
            union {
                u32 state_flags;
                struct {
                    u32 unknown_flag_00 : 1;
                    u32 active : 1;
                    u32 flag_02 : 1;
                    u32 unknown_flag_03 : 1;
                    u32 unknown_flags_04_06 : 3;
                    u32 unknown_flags_07_14 : 8;
                    u32 interaction_state : 2;
                    u32 unknown_flags_17_31 : 15;
                } state_flag_bits;
            };
            u8 unknown_0d4[0xC];
            u32 action_timer;
            u32 unknown_0e4;
            FieldEntity *self;
        };
    };
} FieldEntity;

/*
 * Shared portion of the sprite/model object bound to a field entity. Keeping
 * the command-facing fields typed makes the VM reconstruction readable while
 * the renderer itself remains in assembly.
 */
typedef struct FieldRenderAnimationRange {
    u16 first, end;
    u8 unknown_04[4];
} FieldRenderAnimationRange;
typedef char FieldRenderAnimationRange_SizeCheck[sizeof(FieldRenderAnimationRange) == 8 ? 1 : -1];

typedef struct FieldInteractionBounds {
    s8 minimum_x, maximum_y, width, height, vertical_extent;
} FieldInteractionBounds;
typedef struct FieldBoundsRectangle {
    fx32 min_x, max_x, min_y, max_y;
} FieldBoundsRectangle;
typedef struct FieldBoundsLookup {
    s8 min_x, max_y, width, height;
} FieldBoundsLookup;
typedef char FieldBoundsRectangle_SizeCheck[sizeof(FieldBoundsRectangle) == 16 ? 1 : -1];
typedef char FieldBoundsLookup_SizeCheck[sizeof(FieldBoundsLookup) == 4 ? 1 : -1];

typedef struct FieldAnimationBoundsIndex {
    union { u8 unknown_00[2]; struct { u8 body_index, unknown_01; }; };
    s8 bounds_index;
    u8 unknown_03[3];
} FieldAnimationBoundsIndex;
typedef char FieldInteractionBounds_SizeCheck[sizeof(FieldInteractionBounds) == 5 ? 1 : -1];
typedef char FieldAnimationBoundsIndex_SizeCheck[sizeof(FieldAnimationBoundsIndex) == 6 ? 1 : -1];

typedef struct FieldLocomotionParameters {
    fx32 starting_speed, acceleration, maximum_speed;
    fx32 deceleration, reverse_deceleration, turn_speed_limit;
} FieldLocomotionParameters;
typedef char FieldLocomotionParameters_SizeCheck[sizeof(FieldLocomotionParameters) == 24 ? 1 : -1];

/* The spatial update walks 92-byte navigation records ordered by sort_x. */
typedef struct FieldNavigationVertex {
    fx32 x, y, z, lower_z;
} FieldNavigationVertex;
typedef char FieldNavigationVertex_SizeCheck[sizeof(FieldNavigationVertex) == 16 ? 1 : -1];
typedef struct FieldNavigationSurface {
    union {
        u32 flags;
        struct {
            u32 active : 1, end : 1, unknown_02 : 1, type : 4;
            u32 edge_mask_a : 4, edge_mask_b : 4, edge_mask_c : 4, edge_mask_d : 4;
            u32 vertex_count : 3, slope_axis : 2, unknown_28_31 : 4;
        } bits;
    };
    u32 index;
    union {
        u32 attributes;
        struct {
            u32 unknown_00 : 1, unknown_01 : 1, unknown_02 : 1, unknown_03 : 1;
            u32 unknown_04_15 : 12, unknown_16 : 1, unknown_17_31 : 15;
        } attribute_bits;
    };
    union {
        FieldNavigationVertex vertices[4];
        struct { fx32 sort_x; u8 unknown_10[60]; };
    };
    fx32 min_x, max_x, min_y, max_y;
} FieldNavigationSurface;
typedef char FieldNavigationSurface_SizeCheck[sizeof(FieldNavigationSurface) == 92 ? 1 : -1];

struct FieldRenderObject {
#ifdef __cplusplus
    virtual void unknown_00();
    virtual void unknown_04();
    virtual void unknown_08();
    virtual void unknown_0c();
    virtual void unknown_10();
    virtual void delete_self();
    virtual void unknown_18();
    virtual void unknown_1c();
    virtual void unknown_20();
    virtual void unknown_24();
    virtual void stop();
    virtual void unknown_2c();
    virtual void unknown_30();
    virtual void unknown_34();
    virtual void unknown_38();
    virtual void unknown_3c();
    virtual int unknown_40();
    virtual void unknown_44();
    virtual void unknown_48(int);
    virtual int check_linear_completion(FieldLinearController *controller);
    virtual void unknown_50();
    virtual void update_orbit_movement(FieldOrbitController *controller);
    virtual int advance_orbit_frame(FieldOrbitController *controller);
    virtual void unknown_5c(const void *descriptor, void *controller, s16 animation);
    virtual void reset_controller_work();
    virtual void unknown_64(const void *descriptor, void *controller, s16 animation);
    virtual void set_animation(u8 resource_animation, s16 animation_id, int restart);
    virtual void unknown_6c();
    virtual void unknown_70();
    virtual void unknown_74();
    virtual void set_controller_animation(int animation, int reset);
    virtual void set_palette_animation_paused(s8 slot, int paused);
    virtual void set_palette_animation_mode(s8 slot, u8 mode);
    virtual void unknown_84();
    virtual void bind_palette_animation(s8 slot, s16 animation_id, int restart);
    virtual void request_base_palette_reload();
#else
    FieldRenderObjectVTable *vtable;
#endif
    FieldRenderObject *render_previous, *render_next;
    void *owner;
    u8 screen, unknown_011[3];
    GameSpriteAllocation texture;
    union {
        u8 unknown_02c[0x1C];
        struct { GameSpritePalette *palette; u8 unknown_030[0x18]; };
    };
    const FieldRenderAnimationRange *animation_ranges;
    const void *extra_resource_data;
    const u16 *texture_offsets;
    s16 resource_animation;
    s16 animation_id;
    u8 unknown_058[2];
    s16 animation_speed;
    s16 animation_offset_x, animation_offset_y;
    u16 unknown_060;
    u16 unknown_062;
    u32 unknown_064;
    s32 render_anchor_z;
    u8 unknown_06c[8];
    s16 scale_x, scale_y;
    u16 rotation, unknown_07a;
    union {
        u32 state_flags;
        FieldRenderStateFlags state_flag_bits;
    };
    union {
        u8 unknown_080[0x38];
        struct { u32 unknown_080_word; void *animation_controller; u8 unknown_088[0x30]; };
    };
    u8 transition_active;
    u8 unknown_0b9[3];
    void *transition;
    u8 unknown_0c0[0x40];
    void *components[8];
    u8 unknown_120[0x10];
    union {
        u32 sort_key;
        FieldRenderSortKey sort_key_bits;
        struct { u32 entity_index : 6, vertical_order : 22, layer : 4; } field_sort_key;
    };
    union {
        s8 overlap_priorities[4];
        u8 overlap_priority_bytes[4];
    };
};

/* Renderer presentation retained by a field entity while its renderer is
 * rebound. The optional argument to save/restore selects external storage. */
typedef struct FieldRenderSnapshot {
    struct {
        u16 valid : 1, render_02 : 1, semitransparent : 1, render_05_07 : 3;
        u16 animation_active : 1, animation_suppressed : 1, render_10 : 1, render_11 : 1;
        u16 behavior_state : 4, unknown_14_15 : 2;
    } flags;
    u16 unknown_02;
    s16 animation_id;
    u16 unknown_06;
    u32 unknown_08;
    s16 animation_speed, scale_x, scale_y;
    u16 rotation, unknown_14, unknown_16;
    u32 sort_key;
    u8 overlap_priorities[4];
    struct { u16 finished : 1; s16 loops_remaining : 15; } animation_control;
} FieldRenderSnapshot;
typedef char FieldRenderSnapshot_SizeCheck[sizeof(FieldRenderSnapshot) == 36 ? 1 : -1];

typedef struct FieldPathPoint { s32 x, y; } FieldPathPoint;
typedef char FieldPathPoint_SizeCheck[sizeof(FieldPathPoint) == 8 ? 1 : -1];
typedef struct FieldRoamingOption {
    struct { u16 enabled : 1, direction_mode : 3, delay : 12; } flags;
    u16 unknown_02;
    fx32 speed, distance;
} FieldRoamingOption;
typedef char FieldRoamingOption_SizeCheck[sizeof(FieldRoamingOption) == 12 ? 1 : -1];
/* The spatial copy transfers this 260-byte payload as one aggregate. The
 * path interpretation shares storage with the random roaming settings. */
typedef struct FieldEntityMotionState {
    union {
        u32 unknown_000[65];
        struct {
            struct { u16 enabled : 1, bounce : 1, random_direction : 1, delay : 13; } flags;
            u16 unknown_002;
            union { FieldPathPoint points[32]; fx32 coordinates[64]; };
        } path;
        struct {
            s32 bounds[4];
            FieldRoamingOption options[4];
            u8 unknown_040[196];
        } roaming;
    };
} FieldEntityMotionState;
typedef char FieldEntityMotionState_SizeCheck[sizeof(FieldEntityMotionState) == 260 ? 1 : -1];

/*
 * Runtime extension shared by the scriptable field-entity subclasses. The
 * small FieldEntity base above is still used by its byte-matching constructor.
 */
struct FieldRuntimeEntity {
    FieldEntity base;
    u8 unknown_0ec[8];
    fx32 position_x;
    fx32 position_y;
    fx32 previous_position_x, previous_position_y;
    const void *bounds_resource;
    const FieldInteractionBounds *interaction_bounds;
    const FieldAnimationBoundsIndex *animation_bounds;
    u8 bounds_animation_count;
    s8 bounds_index;
    u8 unknown_112[2];
    fx32 interaction_min_x, interaction_max_x, interaction_min_y, interaction_max_y;
    fx32 interaction_vertical_extent;
    fx32 movement_speed, movement_velocity_x;
    union { fx32 movement_velocity_y; fx32 party_direction_speed; };
    fx32 unknown_134;
    FieldLocomotionParameters locomotion, initial_locomotion;
    fx32 frame_delta_x, frame_delta_y;
    fx32 previous_frame_delta_x, previous_frame_delta_y;
    u16 movement_direction;
    u16 previous_movement_direction, locomotion_state, previous_locomotion_state;
    u8 locomotion_category, previous_locomotion_category, unknown_182[2];
    union {
        u32 base_state_flags;
        FieldBaseStateFlags base_state_flag_bits;
    };
    const s8 *blink_durations;
    FieldEntityVisibilityCallback blink_first_phase, blink_second_phase;
    union {
        u32 saved_presentation_flags;
        FieldSavedPresentationFlags saved_presentation_flag_bits;
    };
    s16 animation_speed;
    u8 resource_index;
    u8 saved_resource_index;
    u8 presentation_state;
    u8 saved_presentation_state;
    u16 animation_id;
    u16 saved_animation_id;
    u16 saved_model_animation;
    u16 saved_animation_frame;
    union {
        s16 unknown_1a6[4];
        struct { s16 screen_x, screen_y, screen_offset_x, screen_offset_y; };
    };
    s16 unknown_1ae[4];
    u16 unknown_1b6;
    union {
        FieldRenderSnapshot render_snapshot;
        struct {
            u16 unknown_1b8, unknown_1ba;
            s16 unknown_1bc;
            u16 unknown_1be;
            u32 unknown_1c0;
            s16 unknown_1c4, unknown_1c6, unknown_1c8;
            u16 unknown_1ca, unknown_1cc, unknown_1ce;
            u32 unknown_1d0;
            struct { u8 unknown_00, unknown_01; } unknown_1d4, unknown_1d6;
            u16 unknown_1d8;
            u8 unknown_1da[2];
        };
    };
    u8 unknown_1dc[4];
    FieldRenderObject *render_object;
    union { u8 unknown_1e4[4]; const struct FieldPrimaryResource *primary_resource; };
    union { void *unknown_1e8; const struct FieldSecondaryResource *secondary_resource; };
    union { void *unknown_1ec; struct FieldPaletteResource *palette_resource; };
    union { u8 unknown_1f0[4]; const struct FieldPrimaryResourceRecord *primary_resource_record; };
    FieldLinearController linear_controller;
    FieldOrbitController orbit_controller;
    union {
        u8 transform_state[0x30];
        FieldTransformController transform_controller;
        struct {
            union {
                u16 transform_flags;
                FieldTransformFlags transform_flag_bits;
            };
            u8 unknown_282[0x2E];
        };
    };
    FieldNavigationSurface *navigation_surfaces;
    const void *navigation_resource;
    s8 support_entity_index, previous_support_entity_index;
    u16 unknown_2ba;
    fx32 position_z;
    fx32 relative_height;
    fx32 support_clearance;
    fx32 previous_position_z;
    fx32 previous_relative_height, previous_support_clearance;
    const FieldInteractionBounds *body_bounds, *navigation_bounds;
    const void *body_bounds_lookup, *navigation_bounds_lookup;
    union {
        s8 unknown_2e4[4];
        struct { s8 body_bounds_index, navigation_bounds_index, body_lookup_index, navigation_lookup_index; };
    };
    s32 body_corner_angles[4];
    fx32 body_min_x;
    fx32 body_max_x;
    fx32 body_min_y;
    fx32 body_max_y;
    fx32 body_vertical_extent;
    union { u8 unknown_30c[0x10]; FieldBoundsRectangle body_lookup_bounds; };
    fx32 body_center_y;
    fx32 navigation_min_x;
    fx32 navigation_max_x;
    fx32 navigation_min_y;
    fx32 navigation_max_y;
    fx32 navigation_vertical_extent;
    union { u8 unknown_334[0x10]; FieldBoundsRectangle navigation_lookup_bounds; };
    fx32 swept_min_x, swept_min_y, swept_max_x, swept_max_y;
    fx32 vertical_velocity, vertical_gravity, vertical_terminal_velocity;
    s16 falling_frames;
    u16 unknown_362;
    fx32 default_vertical_launch_velocity;
    fx32 default_gravity;
    fx32 terminal_fall_velocity;
    s16 unknown_370;
    u16 unknown_372;
    fx32 initial_vertical_launch_velocity, initial_gravity, initial_terminal_fall_velocity;
    fx32 frame_delta_z;
    fx32 previous_frame_delta_z;
    fx32 vertical_start_z;
    union {
        u32 field_state_flags;
        FieldEntityFieldStateFlags field_state_flag_bits;
    };
    union {
        u32 collision_category_mask;
        struct { u32 categories : 11, unknown_11_31 : 21; } collision_category_bits;
    };
    s64 collision_policy;
    union {
        u32 collision_state_flags;
        FieldCollisionStateFlags collision_state_flag_bits;
        struct { u32 unknown_00_15 : 16, unknown_16_17 : 2, unknown_18_19 : 2;
                 u32 unknown_20 : 1, unknown_21 : 1, unknown_22 : 1, unknown_23 : 1, unknown_24_31 : 8; } collision_extra_bits;
        struct { u8 current, saved; u16 unknown_02; } collision_flag_bytes;
    };
    union {
        u32 unknown_3a0;
        struct {
            u32 contact_active : 1, previous_contact_active : 1;
            u32 contact_mask_a : 6, previous_contact_mask_a : 6;
            u32 contact_mask_b : 6, previous_contact_mask_b : 6, unknown_26_31 : 6;
        } unknown_3a0_bits;
    };
    FieldContactDirectionFlags contact_direction_flags;
    s32 unknown_3a8, unknown_3ac;
    FieldNavigationSurface *navigation_cursor;
    fx32 navigation_scan_limit;
    u32 unknown_3b8[4];
    union {
        u32 unknown_3c8;
        struct { u32 unknown_00 : 1, unknown_01_31 : 31; } unknown_3c8_bits;
    };
    union {
        u32 runtime_flags;
        FieldEntityRuntimeFlags runtime_flag_bits;
        struct { u32 unknown_00_24 : 25, auto_priority_mask : 5, unknown_30_31 : 2; } priority_flag_bits;
    };
    s16 unknown_3d0, unknown_3d2;
    s16 unknown_3d4;
    union {
        struct { u16 unknown_00_01 : 2, unknown_02_15 : 14; } unknown_3d6_bits;
        struct { u16 phase : 2, remaining : 14; } bounce_shake;
    };
    u32 unknown_3d8;
    union {
        u16 unknown_3dc;
        struct { u16 path_mode : 1, active : 1, moving : 1, paused : 1, unknown_04_15 : 12; } roaming_state;
    };
    u16 unknown_3de;
    union {
        u32 roaming_flags;
        FieldRoamingFlags roaming_flag_bits;
        struct { u32 unknown_00_01 : 2, count : 4, index : 2, unknown_08_31 : 24; } roaming_options;
    };
    union {
        struct { u32 unknown_00_09 : 10, unknown_10_15 : 6, unknown_16_21 : 6, unknown_22_31 : 10; } unknown_3e4_bits;
        struct { u32 index : 4, count : 5, backward : 1, stop_mask_a : 6, stop_mask_b : 6, unknown_22_31 : 10; } path_state;
    };
    u32 unknown_3e8;
    FieldEntityMotionState unknown_3ec;
    void *unknown_4f0, *unknown_4f4;
    FieldRuntimeEntity *support_entity, *previous_support_entity;
    void *unknown_500, *unknown_504, *unknown_508;
    union { void *unknown_50c; const FieldNavigationSurface *ground_surface; };
    FieldRenderObject *auxiliary_render_object;
    const struct FieldPrimaryResource *auxiliary_primary_resource;
    const struct FieldSecondaryResource *auxiliary_secondary_resource;
    struct FieldPaletteResource *auxiliary_palette_resource;
};

typedef char FieldEntity_SizeCheck[sizeof(FieldEntity) == 0xEC ? 1 : -1];
typedef char FieldRenderObject_SizeCheck[
    sizeof(FieldRenderObject) == 0x138 ? 1 : -1];
typedef char FieldRuntimeEntity_SizeCheck[
    sizeof(FieldRuntimeEntity) == 0x520 ? 1 : -1];

/* Copy state while retaining the destination's vtable and renderer bindings. */
#ifdef __cplusplus
extern "C" {
#endif
void FieldEntity_SaveRenderSnapshot(FieldRuntimeEntity *entity, FieldRenderSnapshot *snapshot);
void FieldEntity_RestoreRenderSnapshot(FieldRuntimeEntity *entity, const FieldRenderSnapshot *snapshot);
FieldEntity *FieldEntity_CopyState(FieldEntity *entity, const FieldEntity *source);
FieldRuntimeEntity *FieldEntity_CopyPlanarState(FieldRuntimeEntity *entity, const FieldRuntimeEntity *source);
FieldRuntimeEntity *FieldEntity_CopySpatialState(FieldRuntimeEntity *entity, const FieldRuntimeEntity *source);
void FieldEntity3D_UpdateScreenPosition(FieldRuntimeEntity *entity, s16 camera_x, s16 camera_y);
void FieldEntity3D_UpdateShadow(FieldRuntimeEntity *entity);
int FieldEntity3D_ShouldShowShadow(FieldRuntimeEntity *entity);
void FieldEntity2D_UpdateAnimationState(FieldRuntimeEntity *entity);
void FieldEntity3D_UpdateAnimationState(FieldRuntimeEntity *entity);
#ifdef __cplusplus
}
#endif

#endif
