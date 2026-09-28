/*
 * Pocket Chomp adult exit (overlay 18, 0x020C5D34-0x020C60BC).
 *
 * An adult leaves by moving directly or by making three diminishing bounces.
 */

#include "pocket_chomp_internal.h"

extern "C" {
u32 PocketChompAdult_BeginExit(PocketChompAdultMotion *adult) {
    PocketChompAttackWorkPrefix *work = data_ov002_020c0710;
    BattleSceneObject *object = adult->object;
    int exit_x;
    if (adult->bits.direction)
        exit_x = work->horizontal_offset - 336;
    else
        exit_x = work->horizontal_offset + 592;
    int delta = exit_x - object->x;
    int distance = delta > 0 ? delta : -delta;
    int speed = Overlay18Attack_GetMotionScaleQ8(work->progress);
    int duration = _s32_div_f(distance << 8, speed);
    BattleSceneObject_MoveBy(object, 1, delta, 0, 0, duration);
    func_ov018_020c2e50(adult, 33026);
    u32 result = (adult->flags & ~0x7c0) | 0x180;
    adult->flags = result;
    return result;
}
}

extern "C" void PocketChompAdult_BounceAway(PocketChompAdultMotion *adult, int bounce)
{
    PocketChompAttackWorkPrefix *work = data_ov002_020c0710;
    BattleSceneObject *object = adult->object;
    if (!bounce) {
        work->flags16.stop = 1;
        /* Plan the horizontal exit using the combined duration of all three
         * arcs. Each trial channel is disabled before starting the real bounce. */
        int duration = 0;
        int height = 200;
        for (int i = 0; i < 3; ++i) {
            int velocity = FX_Sqrt((int)(4096.0 * height)) / 16;
            duration += BattleSceneObject_StartVerticalArc(object, 2, velocity, height,
                i == 0 ? 0 : object->z);
            /* Keep both double operations and the per-bounce truncation. */
            height = (int)((153.6 * height) / 256.0);
            BattleMotionChannel *motion = BattleSceneObject_GetMotionChannel(object, 2);
            motion->callback = 0;
            motion->has_deferred_delta = 0;
        }
        int exit_x;
        if (adult->bits.direction)
            exit_x = (s16)(work->horizontal_offset - 336);
        else
            exit_x = (s16)(work->horizontal_offset + 592);
        BattleSceneObject_MoveTo(object, 1, exit_x, object->y, 0, duration);
        adult->bounce_count = 0;
        BattleModelEffect_SpawnRelative(27, object, 0, 0, 0, 12 - object->effect_anchor_z, 256);
        BattleSpriteEffect_SpawnRelative(12, object, 0, 0, 12 - object->effect_anchor_z, 256);
    }
    if (adult->bounce_count <= 2) {
        func_ov018_020c2e50(adult, 33028);
        int height = 200;
        for (int i = 0; i < bounce; ++i)
            height = (int)((153.6 * height) / 256.0);
        int velocity = FX_Sqrt((int)(4096.0 * height)) / 16;
        BattleSceneObject_StartVerticalArc(object, 2, velocity, height, 0);
        adult->bounce_count = bounce + 1;
        adult->flags = (adult->flags & ~0x7c0) | 0x200;
    } else if (!BattleSceneObject_IsAnimationChannelActive(object, 1)) {
        Overlay18Attack_ResetObjectController((Overlay18AttackObjectController *)adult);
    }
}
