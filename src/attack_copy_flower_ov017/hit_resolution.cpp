/* Copy Flower hit resolution, overlay17, 0x020C4994..0x020C4C54.
 * Resolve the participant input, bonus roll and damage before advancing the
 * enemy snapshot, rating and target. Table rows are ordinary, bonus and failed
 * input; a -1 effect ID suppresses that effect family.
 */
#include "participant_internal.h"
extern "C" {
#include <game/battle_damage.h>
#include <game/battle_feedback.h>
#include <game/overlay010_attack.h>
struct CopyFlowerHitEffects { int sprite, model, reaction, sound; };
extern const CopyFlowerHitEffects data_ov017_020c6cc0[3];
extern const s16 data_ov017_020c6c80[];
extern const int *data_ov017_020c6c84;
extern const s8 data_ov017_020c6c88[][2];
int Overlay10Enemy_IsSelectable(u16 actor_id);
void func_ov017_020c4620(Overlay17Participant *participant);
}

extern "C" void Overlay17Participant_ResolveHit(Overlay17Participant *participant)
{
    u16 target;
    int variant;
    BattleActor *actor;
    int damage;
    Overlay17BattleStateView *work;
    int hit_kind, outcome, failed;
    BattleSceneObject *object;
    int offset_y, offset_x;

    failed = 0;
    target = participant->state.target;
    work = (Overlay17BattleStateView *)data_ov002_020c0710;
    actor = BattleActor_GetById(target);
    object = BattleSceneObject_GetById(target);
    offset_y = object->property_0ff - object->property_0fa;
    offset_x = object->property_0fe;
    if (participant->state.bits.input == 1) {
        if (BattleParty_RollHitBonus(work->party[participant->state.bits.formation], actor)) {
            hit_kind = 6;
            outcome = 1;
        } else {
            hit_kind = 1;
            outcome = 0;
        }
        variant = hit_kind;
        BattleRumble_PlayRepeated(6, 1, 0);
    } else {
        hit_kind = 0;
        outcome = 2;
        variant = -1;
        failed = 1;
    }
    unsigned formation = participant->state.bits.formation;
    int power = formation >= 2 ? work->power[1] : work->power[0];
    damage = Overlay10Attack_CalculateDamage(work->party[formation], power, hit_kind, actor, 0);
    BattleEffect_SetVariant((s16)variant);
    int sprite = data_ov017_020c6cc0[outcome].sprite;
    if (sprite != -1)
        BattleSpriteEffect_SpawnRelative(sprite, object, offset_x, offset_y, 0, 256);
    int model = data_ov017_020c6cc0[outcome].model;
    if (model != -1)
        BattleModelEffect_SpawnRelative(model, object, 0, offset_x, offset_y, 0, 256);
    BattleDamage_ApplyToEnemy(object, offset_x, offset_y, damage,
        data_ov017_020c6cc0[outcome].reaction, 7, 1);
    BattleSound_Play((u16)data_ov017_020c6cc0[outcome].sound, 0, 0, 0);
    Overlay10Enemy_AddScaleSteps((Overlay10EnemyState *)work, target, 1);
    Overlay10Enemy_ApplyProjectedDamage((Overlay10EnemyState *)work, target, (s16)damage);
    BattleFeedback_SpawnVariant(object, offset_x, offset_y, 1);
    Overlay10Attack_ShowRating(data_ov017_020c6c80, data_ov017_020c6c88,
        participant->state.bits.formation, &participant->object, failed, data_ov017_020c6c84);
    if (!Overlay10Enemy_IsSelectable(target)) {
        if (work->setup_bits.random_target)
            work->target = Overlay10Enemy_SelectReactionTarget();
        else
            work->target = Overlay17Attack_SelectNextTarget(work->target);
        if (!work->target)
            func_ov017_020c4620(participant);
    }
    if (!failed)
        ++work->successful_hits;
}
