/* Command eligibility and item-target exclusion (overlay2).
 * Item checks refresh each party member's exclusion flag as a side effect.
 * The separate adult/baby decisions preserve carrying and revive rules.
 */
#include <game/battle_actor.h>
#include <game/battle_context.h>
#include <game/save_data.h>

enum BattleCommandAvailabilityOffset {
    BATTLE_ACTIVE_ACTOR_ID_OFFSET = 0x20,
    BATTLE_SELECTED_ITEM_INDEX_OFFSET = 0x120,
    BATTLE_BROS_ITEM_STATE_OFFSET = 0x130,
    BATTLE_ITEM_TARGET_STATE_OFFSET = 0x134,
    BATTLE_BROS_ITEM_MASK_OFFSET = 0x5A8,
    SAVE_PARTY_FORM_OFFSET = 0x558
};
enum BattleCommandId {
    BATTLE_COMMAND_SLOT_1 = 1,
    BATTLE_COMMAND_SLOT_2 = 2,
    BATTLE_COMMAND_BROS_ITEM = 3,
    BATTLE_COMMAND_ITEM = 4,
    BATTLE_COMMAND_RETREAT = 5
};
enum BattleItemTargetMode {
    BATTLE_ITEM_TARGET_RESTORE_0 = 0,
    BATTLE_ITEM_TARGET_RESTORE_1 = 1,
    BATTLE_ITEM_TARGET_REVIVE = 2,
    BATTLE_ITEM_TARGET_STATUS = 3
};
typedef struct BattleItemTargetState {
    u8 unknown_00[0x0C];
    u32 target_mode_flags;
} BattleItemTargetState;
typedef char BattleItemTargetStatePrefixSize[sizeof(BattleItemTargetState) == 16 ? 1 : -1];

int BattleCommand_IsAvailable(int command_id) {
    BattleActor_GetPartySlot(
        *(u16 *)(gBattleContext + BATTLE_ACTIVE_ACTOR_ID_OFFSET));

    switch (command_id) {
    case BATTLE_COMMAND_SLOT_1:
        return 1;
    case BATTLE_COMMAND_SLOT_2:
        return 1;
    case BATTLE_COMMAND_BROS_ITEM:
        if (*(void **)(gBattleContext +
                       BATTLE_BROS_ITEM_STATE_OFFSET) == 0) {
            return 0;
        }
        return (*(u32 *)(gBattleContext +
                         BATTLE_BROS_ITEM_MASK_OFFSET) &
                (1u << *(s16 *)(gBattleContext +
                               BATTLE_SELECTED_ITEM_INDEX_OFFSET))) != 0;
    case BATTLE_COMMAND_ITEM: {
        BattleItemTargetState *target_state =
            *(BattleItemTargetState **)(
                gBattleContext + BATTLE_ITEM_TARGET_STATE_OFFSET);
        BattleActor *party[4];
        int target_mode;
        int i;

        if (target_state == 0) {
            return 0;
        }
        target_mode =
            (u32)(2 * target_state->target_mode_flags) >> 25;
        party[0] = BattleActor_GetPartySlot(BATTLE_ACTOR_MARIO);
        party[1] = BattleActor_GetPartySlot(BATTLE_ACTOR_LUIGI);
        party[2] = BattleActor_GetPartySlot(BATTLE_ACTOR_BABY_MARIO);
        party[3] = BattleActor_GetPartySlot(BATTLE_ACTOR_BABY_LUIGI);

        party[0]->flags &= ~BATTLE_ACTOR_FLAG_13;
        party[1]->flags &= ~BATTLE_ACTOR_FLAG_13;
        if (*(s16 *)(gSaveData + SAVE_PARTY_FORM_OFFSET) == 2 &&
            target_mode < 4) {
            party[2]->flags &= ~BATTLE_ACTOR_FLAG_13;
            party[3]->flags &= ~BATTLE_ACTOR_FLAG_13;
        } else {
            party[2]->flags |= BATTLE_ACTOR_FLAG_13;
            party[3]->flags |= BATTLE_ACTOR_FLAG_13;
        }

        if (target_mode != BATTLE_ITEM_TARGET_REVIVE) {
            if (party[0]->current_hp <= 0) {
                party[0]->flags |= BATTLE_ACTOR_FLAG_13;
            }
            if (party[1]->current_hp <= 0) {
                party[1]->flags |= BATTLE_ACTOR_FLAG_13;
            }
            if (party[2]->current_hp <= 0) {
                party[2]->flags |= BATTLE_ACTOR_FLAG_13;
            }
            if (party[3]->current_hp <= 0) {
                party[3]->flags |= BATTLE_ACTOR_FLAG_13;
            }
        } else {
            if (party[0]->current_hp > 0) {
                party[0]->flags |= BATTLE_ACTOR_FLAG_13;
            } else {
                party[2]->flags |= BATTLE_ACTOR_FLAG_13;
            }
            if (party[1]->current_hp > 0) {
                party[1]->flags |= BATTLE_ACTOR_FLAG_13;
            } else {
                party[3]->flags |= BATTLE_ACTOR_FLAG_13;
            }
            if (party[2]->current_hp > 0) {
                party[2]->flags |= BATTLE_ACTOR_FLAG_13;
            }
            if (party[3]->current_hp > 0) {
                party[3]->flags |= BATTLE_ACTOR_FLAG_13;
            }
            if (*(s16 *)(gSaveData + SAVE_PARTY_FORM_OFFSET) == 2) {
                if (party[0]->transition_state > 0) {
                    party[2]->flags |= BATTLE_ACTOR_FLAG_13;
                }
                if (party[1]->transition_state > 0) {
                    party[3]->flags |= BATTLE_ACTOR_FLAG_13;
                }
            }
        }

        for (i = 0; i < 4; i++) {
            BattleActor *actor = party[i];

            if ((u32)target_mode <= BATTLE_ITEM_TARGET_RESTORE_1) {
                if (actor->current_hp == actor->max_hp) {
                    actor->flags |= BATTLE_ACTOR_FLAG_13;
                }
            } else if (target_mode == BATTLE_ITEM_TARGET_STATUS &&
                       actor->transition_state < 2 &&
                       actor->force_low_hp_animation == 0 &&
                       actor->status_channel_50 >= 0 &&
                       actor->status_channel_5c >= 0 &&
                       actor->status_channel_68 >= 0) {
                actor->flags |= BATTLE_ACTOR_FLAG_13;
            }
        }

        if (party[0]->flag_bits.excluded_from_targeting &&
            party[1]->flag_bits.excluded_from_targeting &&
            party[2]->flag_bits.excluded_from_targeting &&
            party[3]->flag_bits.excluded_from_targeting) {
            return 0;
        }
        return 1;
    }
    case BATTLE_COMMAND_RETREAT:
        if (((BattleRuntimeFlags *)(gBattleContext +
                                    BATTLE_RUNTIME_FLAGS_OFFSET))
                ->bits.global_property_19) {
            return 0;
        }
        if (BattleActor_GetPartySlot(BATTLE_ACTOR_MARIO)->transition_state > 0) {
            return 0;
        }
        return BattleActor_GetPartySlot(BATTLE_ACTOR_LUIGI)->transition_state <= 0;
    default:
        return 0;
    }
}
