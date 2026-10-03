/*
 * Accelerated motion update (overlay 2, 0x020A43D8-0x020A4518).
 *
 * One frame of an accelerated motion.
 */
 *
 * Exact-size match (320 B). Six register-allocation differences remain:
 * native uses r4/r12/r14/r5/r1/r6 where MWCC allocates r12/r14/r5/r4/r6.
 * Gap documented per AGENTS.md: do not brute-force register allocation.


#include <game/battle_scene.h>

typedef struct BattleAcceleratedMotionParameters {
    s32 acceleration;
    s32 doubled_initial_velocity;
    s16 distance;
    s16 direction_x_q14;
    s16 direction_y_q14;
    s16 direction_z_q14;
} BattleAcceleratedMotionParameters;

void BattleSceneObject_UpdateAcceleratedMotion(
    BattleSceneObject *object, BattleMotionChannel *channel) {
    s32 elapsed_q8;
    BattleAcceleratedMotionParameters *parameters;
    int elapsed_q4;
    int velocity;
    int delta_x;
    int delta_y;
    int delta_z;
    int remaining_distance;

    elapsed_q8 = channel->elapsed_q8;
    parameters = (BattleAcceleratedMotionParameters *)channel->parameters;

    if (elapsed_q8 / 256 == channel->duration) {
        remaining_distance = parameters->distance;
        object->x += parameters->direction_x_q14 * remaining_distance /
                     0x4000;
        object->y += parameters->direction_y_q14 * remaining_distance /
                     0x4000;
        object->z += parameters->direction_z_q14 * remaining_distance /
                     0x4000;
    } else {
        elapsed_q4 = elapsed_q8 / 16;
        velocity = parameters->doubled_initial_velocity +
                   parameters->acceleration * elapsed_q4 / 16;
        delta_x = parameters->direction_x_q14 * (velocity / 32) /
                  4096 * elapsed_q4;
        delta_y = parameters->direction_y_q14 * (velocity / 32) /
                  4096 * elapsed_q4;
        delta_z = parameters->direction_z_q14 * (velocity / 32) /
                  4096 * elapsed_q4;
        object->x += delta_x / 0x40000;
        object->y += delta_y / 0x40000;
        object->z += delta_z / 0x40000;
    }
}
