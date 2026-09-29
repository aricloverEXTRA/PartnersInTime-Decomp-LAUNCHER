/*
 * Shell support movement (overlay 11, 0x020C303C-0x020C3388).
 *
 * The partner's part of the attack: launching away, starting the intercept and
 * scheduling when it happens, with the speed updated each frame so the intercept
 * lands on the intended frame.
 */

#include "shell_internal.h"

extern "C" {
void Overlay11Support_UpdateSpeed(Overlay11Support *support)
{
    if (support->object) {
        support->speed_q8 =
            GameMotion_ClampToLimit(support->speed_q8 + data_ov011_020c5f7e[data_ov002_020c0710->variant][0],
                          data_ov011_020c5f7e[data_ov002_020c0710->variant][0],
                          data_ov011_020c5f80[data_ov002_020c0710->variant][0]);
    }
}

void Overlay11Support_LaunchAway(Overlay11Support *support, int speed_q8)
{
    int dx;
    BattleSceneObject *object = support->object;
    if (object) {
        dx = 320 - object->x;
        Overlay11AttackWork *work = data_ov002_020c0710;
        int duration = (dx << 8) / speed_q8;
        BattleSceneObject_MoveBy(object, 2, dx, 0, 0, duration);
        BattleSceneObject_StartAcceleratedMotionForDuration(support->object, 3, 0, 0, 1, -support->object->z,
                                                            duration, 60, 1);
        if (support->effect_handle) {
            BattleSound_Stop(support->effect_handle - 1);
            support->effect_handle = 0;
        }
        BattleEntity_BindResource(support->object->actor_id, work->controllers[0].actor_resource_id);
        BattleSceneObject_SetAnimationFromComponent(support->object, 41, 0, 2596);
        support->state = 3;
    }
}

void Overlay11Support_StartIntercept(Overlay11Support *support, int duration, int reposition,
                                     int minimum_speed, int animation)
{
    BattleSceneObject *object = support->object;
    int should_reposition = reposition;
    if (object) {
        Overlay11AttackWork *work = data_ov002_020c0710;
        if (reposition) {
            BattleSceneObject_AdjustPosition(
                object, -32 - object->x,
                work->controllers[support->side].actor->actor.scene_object->y - object->y, -object->z);
        }
        BattlePosition position;
        func_ov011_020c2e20(&position, &work->controllers[support->side], &work->projectile);
        int dx = position.x - object->x;
        if ((dx << 8) / duration < minimum_speed) {
            int travel = (dx << 8) / minimum_speed;
            Overlay11Support_ScheduleIntercept(support, travel, duration - travel, should_reposition, -1);
        } else {
            BattleSceneObject_MoveBy(object, 2, dx, 0, 0, duration);
            BattleSceneObject_StartAcceleratedMotionForDuration(
                object, 3, 0, 0, 1, work->particle_config->height - object->z, duration, 44, 1);
            BattleSound_Play(42, 0, 0, 0);
            if (animation != -1)
                BattleSceneObject_SetAnimationFromComponent(object, animation, 0, 2596);
            support->state = 6;
        }
    }
}

void Overlay11Support_ScheduleIntercept(Overlay11Support *support, int duration, int delay, int reposition,
                                        int animation)
{
    support->duration = duration;
    support->delay = delay;
    support->reposition = reposition;
    support->animation = animation;
    support->state = 5;
}
}
