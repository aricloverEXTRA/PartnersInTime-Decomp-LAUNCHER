/*
 * ov002 alpha fade channel start (0x0206F0BC-0x0206F164): opens a motion
 * channel whose callback interpolates the fade's packed from/to bytes, then
 * applies the starting alpha to the primary model. A -1 endpoint resolves
 * to the model's current animation state so the fade continues from it.
 */

#include <nitro.h>
#include <game/battle_scene.h>

extern void func_ov002_0206f164(BattleSceneObject *object,
                                BattleMotionChannel *channel);

int func_ov002_0206f0bc(BattleSceneObject *object, int channel_index, int from,
                        int to, int duration, int argument_5)
{
    s8 *fade;

    if (duration <= 0 || from == to) {
        return BattleModel_SetAlpha(object->primary_model, from, 0);
    }
    fade = (s8 *)BattleSceneObject_BeginMotionChannel(
        object, channel_index, duration, func_ov002_0206f164);
    fade[0] = (from == -1) ? object->primary_model->animation_state_bits.state
                           : from;
    fade[1] = (to == -1) ? object->primary_model->animation_state_bits.state
                         : to;
    return BattleModel_SetAlpha(object->primary_model, fade[0], 0);
}
