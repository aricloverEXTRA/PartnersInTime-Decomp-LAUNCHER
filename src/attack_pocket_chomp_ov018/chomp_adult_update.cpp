/* Adult jump timing, input grades, landing and return.
 * Overlay 18, 0x020C6484-0x020C6918.
 */

#include "pocket_chomp_internal.h"
extern "C" {
#include <game/battle_feedback.h>
#include <game/battle_impact_effect.h>
}
extern "C" {
extern u8 *gBattleContext;
extern u16 data_ov002_020be704[];
void func_ov018_020c60bc(PocketChompAdultMotion *);
void func_ov018_020c5a98(PocketChompAdultMotion *);
void func_ov002_02071938(BattleSceneObject *, BattleSceneObject *, int);

void PocketChompAdult_Update(PocketChompAdultMotion *adult)
{
    PocketChompAttackWork *work = (PocketChompAttackWork *)data_ov002_020c0710;
    BattleSceneObject *object = adult->object;
    switch (adult->bits.phase) {
    case 1:
        if (--adult->entry_timer <= 0)
            PocketChompAdult_BeginJump(adult);
        break;
    case 2:
        if (object->z < object->motion_origin_z) {
            func_ov018_020c2e50(adult, 16386);
            adult->bits.phase = 3;
        }
        /* The falling phase shares the input-window update. */
    case 3: {
        if (--adult->input_timer == 0) {
            if (BattleActor_HasTrait1(work->target_id)) {
                BattleActor_SetStateFlag03(work->target_id);
                BattleActor *target = BattleActor_GetEnemySlot(work->target_id);
                /* Preserve the native forwarded enemy argument; the callee ignores it. */
                BattleParty_ApplyNonfatalDamageFeedback(adult->actor, (int)target, object);
                func_ov018_020c2c14(37, object);
                PocketChompAdult_BounceAway(adult, 0);
                PocketChomp_BeginExitWithSupport(&work->center, work->active_support);
                break;
            }
            func_ov018_020c2e50(adult, 16387);
            BattleObjectPropertyCurve_Start(object, BattleSceneObject_GetById(work->target_id), 16, 0);
        }
        if (!adult->timing.grade) {
            u16 pressed = *(u16 *)(gBattleContext + 260) & 0xc03;
            PocketChompSupport *support = work->active_support;
            if (support && support->object)
                pressed &= ~data_ov002_020be704[support->bits.mode];
            if (pressed == data_ov002_020be704[adult->bits.mode]) {
                for (int grade = 4; grade >= 2; --grade) {
                    const s16 *window = Overlay18Attack_GetInputTimingWindow(work->progress, grade);
                    /* Capture both window bounds before testing the timer. */
                    int early = window[0];
                    int late = window[1];
                    if (-late <= adult->input_timer && adult->input_timer <= early) {
                        adult->timing.grade = grade;
                        break;
                    }
                }
            } else if (pressed) {
                adult->timing.grade = 1;
            }
        }
        const s16 *window = Overlay18Attack_GetInputTimingWindow(work->progress, 2);
        if (adult->input_timer <= -window[1] ||
            (adult->input_timer == 0 && adult->timing.grade)) {
            BattleActor_SetStateFlag03(work->target_id);
            switch (adult->timing.grade) {
            case 0:
            case 1:
                adult->timing.grade = 1;
                func_ov018_020c60bc(adult);
                break;
            default:
                func_ov018_020c60bc(adult);
                break;
            }
            func_ov018_020c5a98(adult);
        }
        break;
    }
    case 4:
        if (object->z < object->motion_origin_z) {
            if (adult->timing.grade != 1)
                func_ov018_020c2e50(adult, 16386);
            adult->bits.phase = 5;
        }
        /* Landing feedback is shared by phases 4 and 5. */
    case 5:
        if (!BattleSceneObject_IsAnimationChannelActive(object, 2)) {
            if (adult->timing.grade == 1)
                PocketChompAdult_SelectEffect((Overlay18AttackObjectController *)adult, 14, 15);
            else
                PocketChompAdult_SelectEffect((Overlay18AttackObjectController *)adult, 16, 17);
            int sound = adult->timing.grade == 1 ? 58 : 57;
            func_ov018_020c2c14(sound, object);
            PocketChompAdult_BeginExit(adult);
        }
        break;
    case 6:
        if (!BattleSceneObject_IsAnimationChannelActive(object, 1)) {
            adult->timing_flags |= 16;
            Overlay18Attack_ResetObjectController((Overlay18AttackObjectController *)adult);
        }
        break;
    case 8:
        if (!BattleSceneObject_IsAnimationChannelActive(object, 2)) {
            PocketChompAdult_SelectEffect((Overlay18AttackObjectController *)adult, 16, 17);
            func_ov018_020c2c14(43, object);
            object->time_step_adjustment_q8 = -256;
            adult->entry_timer = 0;
            adult->bits.phase = 9;
        }
        break;
    case 9:
        if (adult->entry_timer < 0) {
            ++adult->entry_timer;
        } else {
            object->time_step_adjustment_q8 = 0;
            PocketChompAdult_BounceAway(adult, adult->bounce_count);
        }
        break;
    case 13:
        if (!BattleSceneObject_IsAnimationChannelActive(object, 1)) {
            BattleParty_RestoreIdleAnimation(adult->actor, object);
            adult->bits.phase = 0;
        }
        break;
    }
    func_ov002_02071938(adult->variant_object, object, 32);
}
}
