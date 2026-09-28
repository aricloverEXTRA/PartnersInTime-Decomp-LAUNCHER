/* Pocket Chomp phase transitions, launch delay and collision response.
 * Overlay 18, 0x020C4518-0x020C4908.
 */

#include "pocket_chomp_internal.h"
extern "C" {
void func_ov018_020c3b28(PocketChomp *, int);
void func_ov018_020c4174(PocketChomp *, int, PocketChompAdultMotion *, int);
int func_ov018_020c57b0(PocketChompAdultMotion *, PocketChomp *);
extern u16 data_ov018_020c7190[];

void PocketChomp_Update(PocketChomp *center, PocketChompAdultMotion *adult,
                        PocketChompSupport *support, PocketChompSupport *other_support)
{
    PocketChompAttackWorkPrefix *work;
    BattleSceneObject *object = center->object;
    work = data_ov002_020c0710;
    BattleSceneObject_GetActiveModel(object);
    switch (center->bits.phase) {
    case 1:
    case 2:
        if (!center->bits.sound_played) {
            /* The visibility-gated sound helper accepts a full-width ID. */
            int sound = center->bits.rare_variant ? 390 : 389;
            center->bits.sound_played = (u16)func_ov018_020c2c14(sound, center->object);
        }
        func_ov018_020c3ea8(center);
        if (!BattleSceneObject_IsAnimationChannelActive(object, 0)) {
            if (center->bits.phase == 1) {
                work->contact_offset = PocketChompAdult_GetClampedDistance(work->active_adult, center);
                Overlay18Attack_ResetObjectController((Overlay18AttackObjectController *)work->active_adult);
                Overlay10Motion_StopAll(object);
                PocketChomp_Hide(center);
                PocketChompSupport_StopLinkedMotion(support);
                PocketChompSupport_Hide(other_support);
                center->bits.phase = 0;
            } else {
                int speed = Overlay18Attack_GetTravelSpeedQ8(work->progress);
                func_ov018_020c3b28(center, speed);
            }
        }
        break;
    case POCKET_CHOMP_EXIT:
        PocketChomp_UpdateLinkedAnimation(center->linked_object, center);
        if (!BattleSceneObject_IsAnimationChannelActive(object, 1))
            center->bits.phase = 0;
        break;
    case POCKET_CHOMP_ENTRY:
        PocketChomp_UpdateSquash(center);
        if (!BattleSceneObject_IsAnimationChannelActive(object, 1))
            PocketChomp_BeginSmallBounce(center);
        break;
    case POCKET_CHOMP_SMALL_BOUNCE:
        PocketChomp_UpdateSquash(center);
        if (!BattleSceneObject_IsAnimationChannelActive(object, 1)) {
            if (center->timer > 0)
                --center->timer;
            else
                PocketChomp_Reveal(center);
        }
        break;
    case POCKET_CHOMP_REVEAL:
        object->flags.bits.unk_20_23 = 4 * (8 - center->timer) / 8 + 2;
        if (center->timer > 0)
            --center->timer;
        else
            PocketChomp_BeginFall(center);
        break;
    case POCKET_CHOMP_FALL:
        if (!BattleSceneObject_IsAnimationChannelActive(object, 1))
            PocketChomp_BeginRun(center);
        break;
    case POCKET_CHOMP_RUN:
        func_ov018_020c3ea8(center);
        if (!BattleSceneObject_IsAnimationChannelActive(object, 0))
            center->bits.phase = 0;
        break;
    }
    /* These timers also run after the current phase has changed. */
    if (center->bits.launch_pending && --center->timer <= 0) {
        func_ov018_020c4174(center, center->bits.direction, work->active_adult, work->contact_offset);
        center->bits.launch_pending = 0;
        center->bits.phase = 1;
    }
    int collision = func_ov018_020c57b0(adult, center);
    if (collision) {
        switch (collision) {
        case 1:
        case 2:
            PocketChompAdult_BounceAway(adult, 0);
            BattleSound_Play(377, 0, 0, 0);
            BattleRumble_PlayTimed(1, 60, 0);
            BattleSound_Play(data_ov018_020c7190[adult->bits.mode], 0, 0, 0);
            PocketChomp_BeginExitWithSupport(center, support);
            break;
        }
    }
    if (center->bits.variant && --center->alternate_bounce_timer <= 0) {
        center->alternate_bounce_timer = 0;
        center->bits.variant = 0;
        center->velocity_q12 = 0;
        center->object->render_state = 32767;
        PocketChomp_SetAnimation(center, 2, center->bits.direction);
    }
}
}
