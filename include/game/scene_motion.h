#ifndef PIT_GAME_SCENE_MOTION_H
#define PIT_GAME_SCENE_MOTION_H

/*
 * The motion primitives a scene object moves with, and the channels that let
 * several owners move one object at once.
 */
#include <game/scene_script.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*SceneMotionCallback)(SceneObject *, SceneMotionChannel *);
extern SceneObject *gSceneMotionObjectList;
SceneMotionChannel *SceneObject_GetMotionChannel(SceneObject *object, int channel_index);
void SceneObject_StopMotionChannel(SceneObject *object, int channel_index, int apply_deferred_delta);
void SceneObject_UnlinkMotion(SceneObject *object);
s16 *SceneObject_BeginMotionChannel(SceneObject *object, int channel_index, int duration,
                                    SceneMotionCallback callback);
void SceneMotion_UpdateAll(void);
void SceneObject_AdjustPosition(SceneObject *point, int dx, int dy, int dz);
void SceneObject_AddPositionDelta(SceneObject *point, int dx, int dy, int dz);
void SceneObject_MoveByImmediate(SceneObject *point, int dx, int dy, int dz);
void SceneObject_UpdateTravelDistance(SceneObject *point);
void SceneObject_UpdateMoveBy(SceneObject *point, SceneMotionChannel *motion);
void SceneObject_MoveBy(SceneObject *point, int owner, int dx, int dy, int dz, int duration);
void SceneObject_UpdateMoveTo(SceneObject *point, SceneMotionChannel *motion);
void SceneObject_MoveTo(SceneObject *point, int owner, int x, int y, int z, int duration);
void SceneObject_UpdateVerticalMotion(SceneObject *point, SceneMotionChannel *motion);
int SceneObject_StartAcceleratedMotion(SceneObject *object, int channel_index, int direction_x,
                                       int direction_y, int direction_z, int distance, int initial_velocity,
                                       int acceleration, int prefer_longer_duration);
int SceneObject_StartScaledAcceleratedMotion(SceneObject *object, int channel_index, int direction_x,
                                             int direction_y, int direction_z, int distance,
                                             int initial_velocity, int acceleration,
                                             int prefer_longer_duration);
int SceneObject_StartMotionWithPeakDistance(SceneObject *object, int channel_index, int direction_x,
                                            int direction_y, int direction_z, int distance, int peak_distance,
                                            int velocity, int prefer_longer_duration);
int SceneMotion_StartBallistic(SceneObject *object, int channel_index, int direction_x, int direction_y,
                               int direction_z, int distance, int acceleration, int signed_extent,
                               int prefer_longer_duration);
int SceneObject_StartMotionWithVelocityForDuration(SceneObject *object, int channel_index, int direction_x,
                                                   int direction_y, int direction_z, int distance,
                                                   int duration, int velocity);
int SceneObject_StartMotionWithAccelerationForDuration(SceneObject *object, int channel_index,
                                                       int direction_x, int direction_y, int direction_z,
                                                       int distance, int duration, int acceleration);
int SceneObject_StartAcceleratedMotionForDuration(SceneObject *object, int channel_index, int direction_x,
                                                  int direction_y, int direction_z, int distance,
                                                  int duration, int terminal_velocity,
                                                  int positive_acceleration);
void SceneObject_UpdateMoveToObject(SceneObject *object, SceneMotionChannel *channel);
void SceneObject_MoveToObject(SceneObject *object, int channel, int offset_x, int offset_y, int offset_z,
                              int duration, SceneObject *target);
void SceneObject_StartAxisRotation(SceneObject *object, int channel,
    s16 origin_x, s16 origin_y, s16 origin_z,
    s16 axis_end_x, s16 axis_end_y, s16 axis_end_z, s16 angular_speed, s16 angle);
void SceneObject_UpdateAxisRotation(SceneObject *object, SceneMotionChannel *channel);
#ifdef __cplusplus
}
#endif
#endif
