/* Vertical menu selection (overlay2, 0x020999D8..0x02099A74).
 * Unlinked reconstruction draft: button-edge wrapping and held-repeat movement.
 */
#include <game/battle_context.h>
#include <game/battle_effect.h>
enum {
    BATTLE_INPUT_PRESSED_OFFSET = 0x104,
    BATTLE_INPUT_REPEAT_OFFSET = 0x106,
    BATTLE_INPUT_UP = 0x40,
    BATTLE_INPUT_DOWN = 0x80
};

int BattleMenu_UpdateVerticalSelection(int selected_index,
                                       int entry_count) {
    int next_index = selected_index;
    u16 pressed = *(u16 *)(gBattleContext +
                           BATTLE_INPUT_PRESSED_OFFSET);

    if ((pressed & BATTLE_INPUT_UP) != 0 && next_index <= 0) {
        next_index = entry_count - 1;
    } else if ((pressed & BATTLE_INPUT_DOWN) != 0 &&
               entry_count - 1 <= next_index) {
        next_index = 0;
    } else {
        u16 repeated = *(u16 *)(gBattleContext +
                                BATTLE_INPUT_REPEAT_OFFSET);

        if (repeated & BATTLE_INPUT_UP) {
            next_index--;
        }
        if (repeated & BATTLE_INPUT_DOWN) {
            next_index++;
        }
        if (next_index < 0) {
            next_index = 0;
        }
        if (entry_count - 1 < next_index) {
            next_index = entry_count - 1;
        }
    }

    if (selected_index != next_index) {
        BattleSound_Play(1, 0, 0, 0);
    }
    return next_index;
}
