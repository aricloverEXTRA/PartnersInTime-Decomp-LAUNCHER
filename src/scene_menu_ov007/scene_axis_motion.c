/*
 * Scene axis motion (overlay 7, 0x02085998-0x02085B58).
 * Rotation setup and coordinate updates, plus sine-displacement setup.
 * The geometry and sine-update helpers remain native.
 */
#include <game/scene_motion.h>

extern s32 _s32_div_f(s32 numerator, s32 denominator);
extern void func_ov007_0208552c(s16 *position, int angle, int origin_x,
    int origin_y, int origin_z, int axis_end_x, int axis_end_y, int axis_end_z);
extern void func_ov007_02085b58(SceneObject *, SceneMotionChannel *);

typedef struct SceneAxisRotationParameters {
    s16 origin_x, origin_y, origin_z;
    s16 axis_end_x, axis_end_y, axis_end_z;
    s16 angular_speed, angle;
} SceneAxisRotationParameters;

typedef char SceneAxisRotationParameters_SizeCheck[
    sizeof(SceneAxisRotationParameters) == 16 ? 1 : -1];

typedef struct SceneSineMotionParameters {
    s16 direction_x, direction_y, direction_z;
    s16 phase, angular_speed, final_amplitude;
} SceneSineMotionParameters;

typedef char SceneSineMotionParameters_SizeCheck[
    sizeof(SceneSineMotionParameters) == 12 ? 1 : -1];

void SceneObject_StartSineDisplacement(SceneObject *object, int channel,
    int direction_x, int direction_y, int direction_z, int phase,
    int angular_speed, int cycles, int final_amplitude)
{
    int start_phase;
    int speed;
    int duration;
    SceneSineMotionParameters *parameters;

    speed = angular_speed;
    start_phase = phase;
    /* Script arithmetic wraps at 32 bits before signed division. */
    if (speed < 0) {
        speed = (s32)(0u - (u32)speed);
        start_phase = (s32)(0u - (u32)start_phase);
    }
    if (!cycles) {
        duration = 0;
    } else {
        duration = _s32_div_f(
            (s32)((u32)speed - 1u + (((u32)cycles << 16) - (u32)start_phase)),
            speed);
        if (duration <= 0)
            return;
    }
    parameters = (SceneSineMotionParameters *)SceneObject_BeginMotionChannel(
        object, channel, duration, func_ov007_02085b58);
    parameters->direction_x = direction_x;
    parameters->direction_y = direction_y;
    parameters->direction_z = direction_z;
    parameters->phase = start_phase;
    parameters->angular_speed = speed;
    parameters->final_amplitude = final_amplitude;
}

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
