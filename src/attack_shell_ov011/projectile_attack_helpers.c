/*
 * Shell projectile helpers (overlay 11, 0x020C4968-0x020C4C2C).
 *
 * Contact tests, the clamped offset the shell is held at, the finish animation
 * and the actor-resource restore. The battle variant flags decide which of these
 * the Green and the Red Shell use.
 */

#include <game/battle_effect.h>
#include <game/interval.h>
#include <game/battle_object.h>
#include <game/overlay011_attack.h>

enum {
    OVERLAY11_PROJECTILE_OBJECT_ID = 40,
    OVERLAY11_FINISH_RESOURCE_ID = 56,
    OVERLAY11_FINISH_SOUND_ID = 223,
    OVERLAY11_FINISH_STATE = 7,
    OVERLAY11_STATE_MASK = 0x1F,
    OVERLAY11_CONFIGURATION_MASK = 0x0C,
    OVERLAY11_CONTEXT_VARIANT_OFFSET = 0x1D8,
};

extern int BattleAnimation_GetDurationThroughFrame(u8 *resource_data, int animation_id,
                               int animation_frame, int scale);
extern s16 data_ov011_020c6040[];
extern u16 data_ov011_020c5f24[];
extern u8 data_ov011_020c5f20[];
extern s16 data_ov011_020c5f44[];
extern u8 *data_ov000_020c0710;

typedef union Overlay11BattleVariantFlags {
    u8 raw;
    struct {
        u8 alternate : 1;
        u8 unknown_1_7 : 7;
    } bits;
} Overlay11BattleVariantFlags;

static inline int Overlay11Attack_TestModelState(BattleModel *model) {
    return ((int (*)(BattleModel *))((void **)model->vtable)[15])(model);
}

/* Metrowerks emits C functions in reverse source order. */
void Overlay11Attack_RestoreActorResource(Overlay11AttackController *state) {
    BattleSceneObject *object = state->projectile->object;

    if (object->resource->object_data_id != state->actor_resource_id ||
        (object->resource->object_data_id == state->actor_resource_id &&
         Overlay11Attack_TestModelState(
             BattleSceneObject_GetActiveModel(object)))) {
        BattleEntity_BindResource(object->actor_id, state->actor_resource_id);
        BattleSceneObject_SetAnimation(object, 0, -1);
    }
    object->effect_anchor_z = 0;
    BattleSceneObject_GetActiveModel(object)->flags &=
        ~BATTLE_MODEL_ANIMATION_MODE_MASK;
}

int Overlay11Attack_IsIdle(Overlay11AttackController *state) {
    return state->flags.bits.state == 0;
}

void Overlay11Attack_UpdateClampedOffset(Overlay11AttackController *state) {
    s16 *offset = &state->offset_x;
    Overlay11AttackMotionBounds *bounds = state->motion_bounds;

    offset[0] = GameMotion_ClampToLimit(
        offset[0] + bounds->step_x, bounds->step_x, bounds->limit_x);
    offset[1] = GameMotion_ClampToLimit(
        offset[1] + bounds->step_y, bounds->step_y, bounds->limit_y);
}

int Overlay11Attack_TestProjectileContact(Overlay11AttackController *state) {
    u8 *context = data_ov000_020c0710;
    Overlay11AttackProjectile *attack = state->projectile;
    u8 *view_state = context + 0x188;
    Overlay11AttackMotionBounds *bounds = state->motion_bounds;
    BattleSceneObject *actor_object = attack->object;
    BattleSceneObject *projectile =
        BattleSceneObject_GetById(OVERLAY11_PROJECTILE_OBJECT_ID);
    int actor_x = actor_object->x + bounds->collision_offset_x;
    int variant =
        ((Overlay11BattleVariantFlags *)(context +
          OVERLAY11_CONTEXT_VARIANT_OFFSET))->bits.alternate;

    if (GameIntervals_Overlap(
            actor_x - data_ov011_020c5f44[variant], actor_x,
            projectile->x +
                *(s16 *)(view_state + 0x24),
            projectile->motion_origin_x +
                *(s16 *)(view_state + 0x26)) &&
        actor_object->y - 14 < projectile->y &&
        projectile->y < actor_object->y + 14) {
        return 1;
    }
    return 0;
}

void Overlay11Attack_BeginFinishAnimation(Overlay11AttackController *state) {
    BattleSceneObject *object = state->projectile->object;
    BattleModel *model;

    BattleEntity_BindResource(object->actor_id,
                              OVERLAY11_FINISH_RESOURCE_ID);
    BattleSceneObject_SetAnimation(object, 1, -1);
    model = BattleSceneObject_GetActiveModel(object);
    model->flags =
        (model->flags & ~BATTLE_MODEL_ANIMATION_MODE_MASK) |
        BATTLE_MODEL_ANIMATION_MODE_STATUS;
    BattleSound_Play(OVERLAY11_FINISH_SOUND_ID, 0, 0, 0);
    state->flags.raw =
        (state->flags.raw & ~OVERLAY11_STATE_MASK) | OVERLAY11_FINISH_STATE;
}

int Overlay11Attack_GetAnimationDuration(Overlay11AttackProjectile *state) {
    Overlay11AttackAnimationState *animation_state = state->animation_state;
    BattleObjectDataLoadState *load_state =
        BattleObjectData_GetLoadState(state->object_data_id);

    return BattleAnimation_GetDurationThroughFrame(
        load_state->data, 5, animation_state->animation_frame,
        state->animation_scale);
}

void Overlay11Attack_InitializeProjectile(Overlay11AttackProjectile *state,
                                          int unused, int configuration) {
    state->object =
        BattleSceneObject_GetById(OVERLAY11_PROJECTILE_OBJECT_ID);
    state->speed_q8 = data_ov011_020c6040[32 * configuration] << 8;
    state->configuration_parameter = data_ov011_020c5f24[configuration];
    state->flags.raw =
        (state->flags.raw & ~OVERLAY11_CONFIGURATION_MASK) |
        ((data_ov011_020c5f20[configuration] & 3) << 2);
}
