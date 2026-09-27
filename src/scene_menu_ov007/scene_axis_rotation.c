/*
 * Scene axis-rotation channels (overlay 7, 0x02085998-0x02085AB0).
 * Stores the axis endpoints and timing, then adapts the object's coordinates
 * to the native rotation helper's three-halfword position buffer.
 */
#include <game/scene_motion.h>

extern s32 _s32_div_f(s32 numerator, s32 denominator);
extern void func_ov007_0208552c(s16 *position, int angle, int origin_x,
    int origin_y, int origin_z, int axis_end_x, int axis_end_y, int axis_end_z);

typedef struct SceneAxisRotationParameters {
    s16 origin_x, origin_y, origin_z;
    s16 axis_end_x, axis_end_y, axis_end_z;
    s16 angular_speed, angle;
} SceneAxisRotationParameters;

typedef char SceneAxisRotationParameters_SizeCheck[
    sizeof(SceneAxisRotationParameters) == 16 ? 1 : -1];

void SceneObject_UpdateAxisRotation(SceneObject *object, SceneMotionChannel *channel)
{
    SceneAxisRotationParameters *parameters =
        (SceneAxisRotationParameters *)channel->parameters;
    s16 position[3];

    position[0] = object->x;
    position[1] = object->y;
    position[2] = object->base_y;
    func_ov007_0208552c(position,
        parameters->angular_speed * channel->elapsed_q8 / 256,
        parameters->origin_x, parameters->origin_y, parameters->origin_z,
        parameters->axis_end_x, parameters->axis_end_y, parameters->axis_end_z);
    object->x = position[0];
    object->y = position[1];
    object->base_y = position[2];
}

void SceneObject_StartAxisRotation(SceneObject *object, int channel,
    s16 origin_x, s16 origin_y, s16 origin_z,
    s16 axis_end_x, s16 axis_end_y, s16 axis_end_z, s16 angular_speed, s16 angle)
{
    int duration = _s32_div_f(angle * 256, angular_speed);
    SceneAxisRotationParameters *parameters =
        (SceneAxisRotationParameters *)SceneObject_BeginMotionChannel(
            object, channel, duration, SceneObject_UpdateAxisRotation);

    parameters->origin_x = origin_x;
    parameters->origin_y = origin_y;
    parameters->origin_z = origin_z;
    parameters->axis_end_x = axis_end_x;
    parameters->axis_end_y = axis_end_y;
    parameters->axis_end_z = axis_end_z;
    parameters->angular_speed = angular_speed;
    parameters->angle = angle;
}
