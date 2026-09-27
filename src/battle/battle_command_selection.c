/*
 * Command selection (overlay 2, 0x02099E78-0x0209A0C8).
 *
 * Moving the selection around the command wheel. This remains an unlinked
 * reconstruction draft; availability and vertical menus have separate units.
 */

#include <game/battle_actor.h>
#include <game/battle_context.h>
#include <game/battle_effect.h>
#include <game/save_data.h>

enum BattleCommandSelectionConstant {
    BATTLE_INPUT_HELD_OFFSET = 0x102,
    BATTLE_INPUT_PRESSED_OFFSET = 0x104,
    BATTLE_INPUT_REPEAT_OFFSET = 0x106,
    BATTLE_INPUT_RIGHT = 0x10,
    BATTLE_INPUT_LEFT = 0x20,
    BATTLE_INPUT_UP = 0x40,
    BATTLE_INPUT_DOWN = 0x80,
    BATTLE_ACTIVE_ACTOR_ID_OFFSET = 0x20,
    BATTLE_SELECTED_COMMAND_OFFSET = 0x11A,
    BATTLE_SELECTED_ITEM_INDEX_OFFSET = 0x120,
    BATTLE_BROS_ITEM_STATE_OFFSET = 0x130,
    BATTLE_ITEM_TARGET_STATE_OFFSET = 0x134,
    BATTLE_BROS_ITEM_MASK_OFFSET = 0x5A8,
    BATTLE_COMMAND_WHEEL_INTENSITY_OFFSET = 0x6534,
    BATTLE_COMMAND_WHEEL_ENTRIES_OFFSET = 0x6538,
    BATTLE_COMMAND_WHEEL_ENTRY_COUNT_OFFSET = 0x6560,
    BATTLE_COMMAND_WHEEL_ANGULAR_SPEED_OFFSET = 0x6562,
    BATTLE_COMMAND_WHEEL_INPUT_STATE_OFFSET = 0x6566,
    BATTLE_RUNTIME_FLAG_NO_RETREAT = 0x400,
    SAVE_PARTY_FORM_OFFSET = 0x558,
    BATTLE_COMMAND_WHEEL_MAX_ACCELERATION = 5
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

typedef struct BattleCommandSelectionEntry {
    s16 angle;
    u8 unknown_02[6];
} BattleCommandSelectionEntry;

typedef struct BattleItemTargetState {
    u8 unknown_00[0x0C];
    u32 target_mode_flags;
} BattleItemTargetState;

int BattleCommandWheel_UpdateSelection(void) {
    int selected_index = *(s16 *)(
        gBattleContext + BATTLE_SELECTED_COMMAND_OFFSET);
    int entry_count = *(s16 *)(
        gBattleContext + BATTLE_COMMAND_WHEEL_ENTRY_COUNT_OFFSET);

    if (*(s16 *)(gBattleContext +
                 BATTLE_COMMAND_WHEEL_INTENSITY_OFFSET) < 1) {
        return 0;
    }
    if (entry_count <= 1) {
        return 0;
    }

    if ((*(u16 *)(gBattleContext + BATTLE_INPUT_HELD_OFFSET) &
         (BATTLE_INPUT_LEFT | BATTLE_INPUT_RIGHT)) != 0) {
        u16 input_state = *(u16 *)(
            gBattleContext + BATTLE_COMMAND_WHEEL_INPUT_STATE_OFFSET);
        unsigned int acceleration =
            (u32)(input_state << 23) >> 24;

        if (acceleration < BATTLE_COMMAND_WHEEL_MAX_ACCELERATION) {
            *(u16 *)(gBattleContext +
                     BATTLE_COMMAND_WHEEL_INPUT_STATE_OFFSET) =
                (input_state & 0xFE01) |
                (2 * (u8)(acceleration + 1));
        } else {
            *(u16 *)(gBattleContext +
                     BATTLE_COMMAND_WHEEL_INPUT_STATE_OFFSET) =
                (input_state & 0xFE01) | 10;
        }
    } else {
        u16 input_state = *(u16 *)(
            gBattleContext + BATTLE_COMMAND_WHEEL_INPUT_STATE_OFFSET);
        unsigned int acceleration =
            (u32)(input_state << 23) >> 24;

        if (acceleration != 0) {
            *(u16 *)(gBattleContext +
                     BATTLE_COMMAND_WHEEL_INPUT_STATE_OFFSET) =
                (input_state & 0xFE01) |
                (2 * (u8)(acceleration - 1));
        }
    }

    *(s16 *)(gBattleContext +
             BATTLE_COMMAND_WHEEL_ANGULAR_SPEED_OFFSET) =
        200 /
        (entry_count *
         (((u32)(*(u16 *)(
                       gBattleContext +
                       BATTLE_COMMAND_WHEEL_INPUT_STATE_OFFSET) << 23) >>
           24) + 3));

    {
        BattlePartyActor *actor =
            (BattlePartyActor *)BattleActor_GetPartySlot(
                *(u16 *)(gBattleContext +
                         BATTLE_ACTIVE_ACTOR_ID_OFFSET));

        if (*(u16 *)(gBattleContext +
                     BATTLE_COMMAND_WHEEL_INPUT_STATE_OFFSET) & 1) {
            return 0;
        }
        if (actor->state_flags.bits.target_selection_locked) {
            return 0;
        }
    }

    if (*(u16 *)(gBattleContext + BATTLE_INPUT_HELD_OFFSET) &
        BATTLE_INPUT_LEFT) {
        selected_index--;
    }
    if (*(u16 *)(gBattleContext + BATTLE_INPUT_HELD_OFFSET) &
        BATTLE_INPUT_RIGHT) {
        selected_index++;
    }

    while (selected_index < 0) {
        int i = 0;

        selected_index += entry_count;
        do {
            BattleCommandSelectionEntry *entries =
                (BattleCommandSelectionEntry *)(
                    gBattleContext +
                    BATTLE_COMMAND_WHEEL_ENTRIES_OFFSET);

            entries[i].angle -= 256;
            i++;
        } while (i < entry_count);
    }
    while (selected_index >= entry_count) {
        int i = 0;

        selected_index -= entry_count;
        do {
            BattleCommandSelectionEntry *entries =
                (BattleCommandSelectionEntry *)(
                    gBattleContext +
                    BATTLE_COMMAND_WHEEL_ENTRIES_OFFSET);

            entries[i].angle += 256;
            i++;
        } while (i < entry_count);
    }

    if (*(s16 *)(gBattleContext +
                 BATTLE_SELECTED_COMMAND_OFFSET) == selected_index) {
        return 0;
    }
    BattleSound_Play(8, 0, 0, 0);
    *(s16 *)(gBattleContext + BATTLE_SELECTED_COMMAND_OFFSET) =
        selected_index;
    return 1;
}
