/*
 * Flexible link heading, reset and reversal (overlay 2, 0x020BADD8-0x020BAF18).
 *
 * Returns the accumulated heading of the trailing link segments, resets a
 * flexible link, or reverses it toward an available adult.
 */

#include <game/battle_object_link.h>
#include <game/battle_actor.h>
/* Clear horizontal segment motion before reversing the link velocity.
 * Keep the current actor if the other adult cannot receive the interaction. */
void BattleFlexibleLink_Reverse(BattleSceneObject *object, int velocity)
{
    BattleFlexibleLinkState *state = FLEXIBLE_LINK_STATE(object);
    int i = 0;
    if (state->count > 0) {
        BattleObjectLinkSegment *point = state->segments;
        do {
            point->velocity_x = 0;
            ++i;
            ++point;
        } while (i < state->count);
    }
    state->velocity = -velocity;
    if (state->actor == 56) {
        if (BattleActor_CanReceiveStatus(BattleActor_GetById(57))) state->actor = 57;
    } else {
        if (BattleActor_CanReceiveStatus(BattleActor_GetById(56))) state->actor = 56;
    }
}

void BattleFlexibleLink_Reset(BattleSceneObject *object, int unused) {
    BattleFlexibleLinkState *s = FLEXIBLE_LINK_STATE(object);
    s->unknown_d0 = 1;
    s->flags.flag_01 = 0;
    s->flags.flag_00 = 0;
    s->actor = 0;
}

/* Angle of the trailing segment displacements: y accumulates velocity_y -
 * velocity_z so the isometric z axis folds into the screen-plane heading. */
int func_ov002_020badd8(BattleSceneObject *object, int remaining_segment_offset)
{
    BattleFlexibleLinkState *state = FLEXIBLE_LINK_STATE(object);
    int sum_x = 0, sum_heading = 0;
    int i = state->count - remaining_segment_offset;
    BattleObjectLinkSegment *segment;

    if (i < state->count) {
        segment = &state->segments[i];
        do {
            sum_x += segment->velocity_x;
            sum_heading += segment->velocity_y - segment->velocity_z;
            i++;
            segment++;
        } while (i < state->count);
    }
    return FX_Atan2Idx(sum_heading, sum_x);
}
