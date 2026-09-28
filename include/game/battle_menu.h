#ifndef PIT_GAME_BATTLE_MENU_H
#define PIT_GAME_BATTLE_MENU_H

#ifdef __cplusplus
extern "C" {
#endif

/* Update a row in a nonempty battle item list; play a sound only on movement. */
int BattleMenu_UpdateVerticalSelection(int selected_index, int entry_count);

#ifdef __cplusplus
}
#endif
#endif
