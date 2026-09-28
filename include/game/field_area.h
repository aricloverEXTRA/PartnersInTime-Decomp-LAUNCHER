#ifndef PIT_GAME_FIELD_AREA_H
#define PIT_GAME_FIELD_AREA_H

#include <game/field_script_manager.h>
#include <game/field_bounds_records.h>
#include <game/field_navigation.h>
#include <game/field_resources.h>

typedef struct FieldVariablePlacement FieldVariablePlacement;
struct FieldPaletteCrossfade;
struct FieldLayerMotion;

/* The overworld area: the room that is loaded, the collision and navigation
   data read for it, the script manager driving it, and the pending room change.
   One FieldAreaContext exists at a time and everything in the field subsystem
   reaches the world through it.

   Only the parts that have been recovered are typed; the unknown_<offset> gaps
   keep the known fields pinned to their original addresses. */

/* A queued room change: where the party reappears, which script runs on
   arrival, and what stays locked while the screen is faded out. */
typedef struct FieldAreaTransition {
    struct {
        u32 pending:1, fading:1, party_mask:2, direction_0:3, direction_1:3, control_locked:1;
        s32 bgm:8;
        u32 movement_modes:8, unknown_27_31:5;
    } flags;
    u16 room;
    s16 script, x[2], y[2], z[2];
} FieldAreaTransition;
typedef char FieldAreaTransition_SizeCheck[sizeof(FieldAreaTransition)==20?1:-1];
/* A message window moving to a target position, in fixed point. */
typedef struct FieldMessageWindowSlide {
    struct {
        u32 active : 1, fixed_duration : 1, window : 8;
        u32 elapsed : 10, duration : 10, unknown_30_31 : 2;
    } flags;
    fx32 x, y, previous_x, previous_y, target_x, target_y, velocity_x, velocity_y;
} FieldMessageWindowSlide;
typedef char FieldMessageWindowSlide_SizeCheck[sizeof(FieldMessageWindowSlide) == 36 ? 1 : -1];

/* Two queued item notices. Text is copied with native FF 0A 00 termination;
   callers must supply a name that fits the 64-byte formatted buffer. */
typedef struct FieldNotification {
    struct { u16 active : 1, pending : 1, reserved : 14; } flags;
    s16 y;
    u8 text[64];
} FieldNotification;
typedef char FieldNotification_SizeCheck[sizeof(FieldNotification) == 68 ? 1 : -1];

/* The loaded room. The record pointers are read-only data from the room
   archive; the runtime arrays beside them are the mutable state built from it
   (boundaries that can be switched off, navigation surfaces being walked). */
typedef struct FieldAreaContext {
    const void *vtable;
    void *owner;
    union {
        u8 unknown_0008[34];
        struct {
            u8 unknown_0008_prefix[32];
            s16 startup_script; /* -1 selects the room header's default. */
        };
    };
    union {
        struct { s8 unknown_2a; u8 unknown_002b[8913]; };
        struct {
            s8 notification_index;
            u8 notification_timer;
            FieldNotification notifications[2];
            u8 unknown_00b4[8776];
        };
    };
    const FieldQuadRegionRecord *quad_records;
    u8 unknown_2300[188];
    const FieldNavigationResource *navigation_resource;
    const FieldNavigationRecord *navigation_records;
    FieldNavigationSurface *navigation_surfaces;
    u32 navigation_surface_count;
    const FieldBoundaryRecord *boundary_records;
    FieldBoundaryRuntime *boundaries;
    u32 boundary_count;
    const FieldPairedBoundsRecord *paired_records;
    u8 unknown_23dc[16];
    void *archive;
    u16 unknown_23f0, unknown_23f2;
    u16 room_id;
    struct {
        u8 main : 4, temporary : 4;
    } heaps;
    u8 unknown_23f7[1];
    struct {
        u16 screen : 1, party_active : 1, mode : 2, unknown_04_05 : 2, unknown_06 : 1;
        u16 unknown_07_12 : 6, unknown_13 : 1, unknown_14 : 1, unknown_15 : 1;
    } flags;
    union { s8 area_value; s8 camera_target_index; };
    union { s8 unknown_23fb; s8 pending_camera_target; };
    union { s16 unknown_23fc; s16 camera_completion_script; };
    s8 unknown_23fe;
    u8 unknown_23ff[1];
    s16 unknown_2400;
    struct {
        u8 unknown_00 : 1, unknown_01 : 1;
        s8 unknown_02_07 : 6;
    } unknown_2402;
    u8 unknown_2403;
    u16 unknown_2404, unknown_2406;
    s16 unknown_2408, brightness, unknown_240c;
    u16 unknown_240e;
    struct {
        u32 unknown_00 : 1, unknown_01 : 1, unknown_02_07 : 6, unknown_08_13 : 6;
        u32 unknown_14_18 : 5, unknown_19_23 : 5, unknown_24_31 : 8;
    } unknown_2410;
    u16 unknown_2414, unknown_2416;
    s16 unknown_2418[6];
    s32 unknown_2424[4];
    s32 camera_x, camera_y;
    s32 unknown_243c[4];
    s16 unknown_244c[4];
    s32 unknown_2454[4];
    s16 unknown_2464[2];
    s32 unknown_2468[2];
    s32 unknown_2470[6];
    struct {
        u16 unknown_00 : 1, unknown_01 : 1, unknown_02_15 : 14;
    } unknown_2488;
    u16 unknown_248a;
    s32 unknown_248c[4];
    FieldMessageWindowSlide window_slide;
    u32 unknown_24c0;
    u16 input_masks[2];
    union {
        u8 unknown_24c8[16];
        struct {
            u16 scripted_held, scripted_pressed;
            u8 unknown_24cc[4];
            u16 scripted_held_mask, scripted_pressed_mask;
            u8 unknown_24d4[4];
        };
    };
    struct {
        u16 unknown_00_13 : 14, visible : 1, moving : 1;
    } hud;
    s16 unknown_24da, unknown_24dc;
    u8 unknown_24de[10];
    u32 paired_count;
    FieldPairedBoundsRuntime *paired_bounds;
    union { u32 unknown_24f0; u32 quad_region_count; };
    union {
        u8 unknown_24f4[8];
        struct { FieldQuadRegion *quad_regions; const FieldVariablePlacement *variable_records; };
    };
    union { void *auxiliary; struct FieldSystem *system; };
    union { FieldNavigationChangeManager *navigation_changes; struct FieldBackground *background; };
    u8 unknown_2504[8];
    union {
        u8 unknown_250c[44];
        struct { u8 unknown_250c_prefix[40]; void *unknown_2534; };
    };
    u8 unknown_2538[40];
    union { void *party_order; struct FieldTimer *timer; };
    union { void *unknown_2564; struct FieldModelAnimation *model_animation; };
    union { void *unknown_2568; struct FieldSpriteAnimation *sprite_animation; };
    struct FieldPartyManager *party;
    FieldScriptManager scripts;
    union { u8 unknown_29d4[4]; struct FieldAreaContext *paired_area; };
    union {
        struct FieldRuntimeEntity *entities[36];
        struct {
            struct FieldRuntimeEntity *entity_slots[32], *first_entity, *last_entity;
            union {
                void *unknown_2a60[2];
                struct { struct FieldRuntimeEntity *first_render_entity, *last_render_entity; };
            };
        };
    };
    void *shared_resources;
    void *unknown_2a6c;
    void *unknown_2a70;
    union {
        u8 unknown_2a74[180];
        struct {
            FieldPaletteResource effect_palettes[3];
            FieldRenderObject *effect_models[23];
            u8 unknown_2b18[16];
        };
        struct { u8 unknown_2a74_prefix[0xA4]; struct FieldRenderObject *animation_models[4]; };
    };
    u8 effect_owners[8];
    union {
        u16 state;
        struct {
            u16 unknown_00_01 : 2, unknown_02_04 : 3, unknown_05 : 1, unknown_06 : 1, unknown_07_15 : 9;
        } state_bits;
    };
    union { u8 unknown_2b32[2]; struct { u8 entity_count, variable_count; }; };
    s8 special_resources[6];
    u8 unknown_2b3a[14];
    FieldAreaTransition transition;
    u8 unknown_2b5c[52];
    union { u8 unknown_2b90[52]; struct { u16 state:2, unknown:14; } unknown_2b90_bits; };
    union { u8 unknown_2bc4[8]; struct { struct FieldLayerMotion *layer_motion; void *unknown_2bc8; }; };
    struct FieldPaletteCrossfade *palette_crossfade;
} FieldAreaContext;
typedef char FieldAreaContext_SizeCheck[sizeof(FieldAreaContext) == 11216 ? 1 : -1];
typedef char FieldAreaContext_StartupScriptOffsetCheck[
    (u32)&((FieldAreaContext *)0)->startup_script == 40 ? 1 : -1];
typedef char FieldAreaContext_EffectPalettesOffsetCheck[
    (u32)&((FieldAreaContext *)0)->effect_palettes == 0x2a74 ? 1 : -1];
typedef char FieldAreaContext_EffectModelsOffsetCheck[
    (u32)&((FieldAreaContext *)0)->effect_models == 0x2abc ? 1 : -1];
typedef char FieldAreaContext_Unknown2534OffsetCheck[
    (u32)&((FieldAreaContext *)0)->unknown_2534 == 0x2534 ? 1 : -1];

typedef struct FieldAreaSnapshot {
    u16 room_id;
    struct {
        u16 party_active : 1, mode : 2, unknown_03 : 1;
        s16 area_value : 8;
        u16 unknown_12_14 : 3, hud_visible : 1;
    } flags;
    s32 camera_x, camera_y;
    u32 unknown_0c;
} FieldAreaSnapshot;
typedef char FieldAreaSnapshot_SizeCheck[sizeof(FieldAreaSnapshot) == 16 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
void FieldArea_ApplyQueuedMusic(FieldAreaContext *area);
void FieldArea_BindPartyMembers(FieldAreaContext *area, int side, int force);
void FieldArea_ReverseEffects(FieldAreaContext *area,int index);
void FieldArea_SetEffectsPaused(FieldAreaContext *area,int index,u8 paused);
int FieldArea_AreEffectsActive(FieldAreaContext *area,int index);
void FieldArea_ReadInput(FieldAreaContext *area, int mask, u16 *held, u16 *pressed, u8 mask_script);
int FieldArea_GetCommonInputMask(FieldAreaContext *area);
void FieldArea_OffsetCameraOrigin(FieldAreaContext *area, fx32 x, fx32 y);
void FieldArea_UpdateAlphaBlend(FieldAreaContext *area);
void FieldArea_UpdateBrightness(FieldAreaContext *area);
void FieldArea_ReverseBrightness(FieldAreaContext *area);
void FieldArea_StartBrightness(FieldAreaContext *area, s16 initial, s16 target, u16 frames);
int FieldArea_HasOpenMessageWindow(FieldAreaContext *area, int window);
int FieldArea_HasNonClosingWindow(FieldAreaContext *area, int window);
void FieldArea_CloseMessageWindows(FieldAreaContext *area, int window);
void FieldArea_CenterCameraOnEntity(FieldAreaContext *area, struct FieldRuntimeEntity *entity, fx32 x, fx32 y);
void FieldArea_SetCameraPosition(FieldAreaContext *area, fx32 x, fx32 y);
void FieldArea_UpdateScriptsWhenIdle(FieldAreaContext *area);
void FieldArea_StartScripts(FieldAreaContext *area);
void FieldArea_SaveQuadRegionState(FieldAreaContext *area);
void FieldArea_InitializeQuadRegions(FieldAreaContext *field);
void FieldArea_CreateVariableEntities(FieldAreaContext *field);
void FieldArea_CreatePlacedEntities(FieldAreaContext *area);
void FieldArea_UpdateEntities(FieldAreaContext *area);
void FieldArea_UpdateEntityVisibility(FieldAreaContext *area);
void FieldArea_BeginFrame(FieldAreaContext *area);
void FieldArea_SaveEntityRenderSnapshots(FieldAreaContext *area);
/* Keep entity state for a cached scene, or free the complete room. */
void FieldArea_ReleaseRenderResources(FieldAreaContext *area);
void FieldArea_ReleaseRoomResources(FieldAreaContext *area);
void FieldArea_ResetEntityRenderOrder(FieldAreaContext *area);
void FieldArea_ResetEntityUpdateOrder(FieldAreaContext *area);
FieldAreaContext *FieldArea_CopyState(FieldAreaContext *field, const FieldAreaContext *source);
void FieldArea_UpdateGraphics(FieldAreaContext *field);
void FieldArea_LoadWindowSprites(FieldAreaContext *field);
void FieldArea_UpdateNotifications(FieldAreaContext *area);
void FieldArea_ClearNotifications(FieldAreaContext *area);
void FieldArea_CloseNotification(FieldAreaContext *area, int index);
void FieldArea_QueueNotification(FieldAreaContext *area, const u8 *text, int quantity, int y);
void FieldArea_UpdateMessageWindowSlide(FieldAreaContext *area);
int FieldArea_UpdateMessageWindowSlideCompletion(FieldAreaContext *area);
void FieldArea_StartMessageWindowSlideBySpeed(FieldAreaContext *area, int window, fx32 x, fx32 y, fx32 speed);
void FieldArea_UpdateMessageWindowClip(FieldAreaContext *area, int window);
void FieldArea_SetMessageWindowClipEnabled(FieldAreaContext *area, int window, int enabled);

FieldAreaContext *FieldArea_Construct(FieldAreaContext *field);
FieldAreaContext *FieldArea_Destroy(FieldAreaContext *field);
void FieldArea_CaptureSnapshot(const FieldAreaContext *field, FieldAreaSnapshot *snapshot);
#ifdef __cplusplus
}
#endif
#endif
