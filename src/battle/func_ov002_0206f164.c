/*
 * ov002 alpha fade motion callback (0x0206F164-0x0206F1B8): runs each frame
 * on a motion channel whose first parameter packs the fade's from-byte and
 * to-byte, and stores the interpolated alpha on the object's primary model
 * for the rest of the channel's duration.
 */

#include <nitro.h>
#include <game/battle_scene.h>

extern s32 _s32_div_f(s32 numerator, s32 denominator);

void func_ov002_0206f164(BattleSceneObject *object,
                         BattleMotionChannel *channel)
{
    s8 *fade = (s8 *)&channel->parameters[0];
    s8 from = fade[0];
    s32 factor = _s32_div_f(channel->elapsed_q8 << 4, channel->duration);
    s8 to = fade[1];
    s32 delta = to - from;
    s32 alpha = from + (delta * factor) / 4096;

    BattleModel_SetAlpha(object->primary_model, alpha, 0);
}
