/* Vertical menu selection (overlay2, 0x020999D8..0x02099A74).
 * New presses wrap at the ends; held-button repeats clamp to the list.
 */
#include <game/battle_context.h>
#include <game/battle_menu.h>
#include <game/battle_sound.h>
enum {
    BATTLE_INPUT_PRESSED_OFFSET = 0x104,
    BATTLE_INPUT_REPEAT_OFFSET = 0x106,
    BATTLE_INPUT_UP = 0x40,
    BATTLE_INPUT_DOWN = 0x80
};

int BattleMenu_UpdateVerticalSelection(int selected_index,
                                       int entry_count) {
    int original_index = selected_index;
    u16 pressed = *(u16 *)(gBattleContext +
                           BATTLE_INPUT_PRESSED_OFFSET);

    if ((pressed & BATTLE_INPUT_UP) != 0 && selected_index <= 0) {
        selected_index = entry_count - 1;
    } else if ((pressed & BATTLE_INPUT_DOWN) != 0 &&
               entry_count - 1 <= selected_index) {
        selected_index = 0;
    } else {
        u16 repeated = *(u16 *)(gBattleContext +
                                BATTLE_INPUT_REPEAT_OFFSET);

        if (repeated & BATTLE_INPUT_UP) {
            selected_index--;
        }
        if (repeated & BATTLE_INPUT_DOWN) {
            selected_index++;
        }
        if (selected_index < 0) {
            selected_index = 0;
        }
        if (entry_count - 1 < selected_index) {
            selected_index = entry_count - 1;
        }
    }

    if (original_index != selected_index) {
        BattleSound_Play(1, 0, 0, 0);
    }
    return selected_index;
}
