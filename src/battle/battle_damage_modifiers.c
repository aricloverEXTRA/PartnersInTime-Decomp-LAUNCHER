/*
 * Damage equipment modifiers (overlay 2, 0x0209BCCC-0x0209BF38).
 *
 * Applies the equipped items' modifiers to a damage figure.
  *
 * Exact-size match. BattleDamage_ApplyEquipmentModifiers (620 B). Twelve register-allocation differences. Gap documented per AGENTS.md: do not brute-force register allocation.
 */

#include <game/battle_actor.h>
#include <game/battle_damage.h>
#include <game/item.h>
#include <game/save_data.h>

enum BattleDamageEquipmentConstant {
    SAVE_EQUIPMENT_RECORD_SIZE = 0x24,
    SAVE_EQUIPMENT_EFFECT_OFFSET = 0x418,
    SAVE_USABLE_ITEM_COUNTS_OFFSET = 0x48E,
    BATTLE_USABLE_ITEM_COUNT = 14,
    BATTLE_DAMAGE_PERCENT_BASE = 100,
    BATTLE_DAMAGE_MAX = 999
};

enum BattleDamageEquipmentEffect {
    EQUIPMENT_EFFECT_CONDITIONAL_ATTACK_250 = 0x300D,
    EQUIPMENT_EFFECT_LOW_HP_ATTACK_250 = 0x3014,
    EQUIPMENT_EFFECT_DEFENSE_25 = 0x3015,
    EQUIPMENT_EFFECT_ATTACK_150 = 0x3016,
    EQUIPMENT_EFFECT_INVENTORY_ATTACK = 0x301D,
    EQUIPMENT_EFFECT_DAMAGE_50 = 0x301F,
    EQUIPMENT_EFFECT_DAMAGE_150 = 0x3020
};

int BattleDamage_ApplyEquipmentModifiers(u32 attacker_id, u32 defender_id,
                                         int scale_q8, int condition_active) {
    int damage = BattleDamage_CalculateBase(attacker_id, defender_id, scale_q8);

    if (defender_id >= BATTLE_ACTOR_PARTY_FIRST &&
        defender_id < BATTLE_ACTOR_PARTY_FIRST + BATTLE_ACTOR_PARTY_COUNT) {
        BattleActor *defender = BattleActor_GetPartySlot(defender_id);
        int damage_percent = BATTLE_DAMAGE_PERCENT_BASE;
        u8 *equipment_record =
            gSaveData + SAVE_EQUIPMENT_RECORD_SIZE *
                            (u8)*(u16 *)defender->resource_slot;
        int equipment_effect =
            equipment_record[SAVE_EQUIPMENT_EFFECT_OFFSET] |
            ITEM_ID_BADGE_TAG;

        switch (equipment_effect) {
        case EQUIPMENT_EFFECT_DAMAGE_50:
            damage_percent = 50;
            break;
        case EQUIPMENT_EFFECT_DEFENSE_25:
            damage_percent = 25;
            break;
        case EQUIPMENT_EFFECT_DAMAGE_150:
            damage_percent = 150;
            break;
        }
        damage = (damage * damage_percent + 50) / 100;
    }

    if (attacker_id >= BATTLE_ACTOR_PARTY_FIRST &&
        attacker_id < BATTLE_ACTOR_PARTY_FIRST + BATTLE_ACTOR_PARTY_COUNT) {
        BattleActor *attacker = BattleActor_GetPartySlot(attacker_id);
        u8 *save_cursor = gSaveData;
        int damage_percent = BATTLE_DAMAGE_PERCENT_BASE;
        int equipment_index =
            (u8)*(u16 *)attacker->resource_slot;
        u8 *equipment_record =
            save_cursor + SAVE_EQUIPMENT_RECORD_SIZE *
                              equipment_index;
        int equipment_effect =
            equipment_record[SAVE_EQUIPMENT_EFFECT_OFFSET] |
            ITEM_ID_BADGE_TAG;

        switch (equipment_effect) {
        case EQUIPMENT_EFFECT_LOW_HP_ATTACK_250:
            if (BattleActor_IsHpAtMostQuarter(attacker)) {
                damage_percent = 250;
            }
            break;
        case EQUIPMENT_EFFECT_CONDITIONAL_ATTACK_250:
            if (condition_active == 1) {
                damage_percent = 250;
            }
            break;
        case EQUIPMENT_EFFECT_ATTACK_150:
            damage_percent = 150;
            break;
        case EQUIPMENT_EFFECT_INVENTORY_ATTACK: {
            int item_index;
            int eligible_item_count = 0;

            for (item_index = 0; item_index < BATTLE_USABLE_ITEM_COUNT;
                 item_index++) {
                u16 item_id = item_index | ITEM_ID_USABLE_TAG;
                UsableItemRecord *item;
                u32 item_category;

                if ((item_id & ITEM_ID_USABLE_TAG) != ITEM_ID_USABLE_TAG) {
                    item = 0;
                } else {
                    item = &gItemRecords[item_id & ITEM_ID_INDEX_MASK];
                }
                item_category = (u32)(2 * item->effect_flags) >> 25;
                if (item_category <= 2) {
                    eligible_item_count += *(s8 *)(
                        save_cursor + SAVE_USABLE_ITEM_COUNTS_OFFSET);
                }
                save_cursor++;
            }

            damage_percent = 300 - 8 * eligible_item_count;
            if (damage_percent < BATTLE_DAMAGE_PERCENT_BASE) {
                damage_percent = BATTLE_DAMAGE_PERCENT_BASE;
            }
            break;
        }
        case EQUIPMENT_EFFECT_DAMAGE_50:
            damage_percent = 50;
            break;
        case EQUIPMENT_EFFECT_DAMAGE_150:
            damage_percent = 150;
            break;
        }

        damage = (damage * damage_percent + 50) / 100;
    }

    if (damage > BATTLE_DAMAGE_MAX) {
        damage = BATTLE_DAMAGE_MAX;
    }
    return damage;
}
