/*
 * Pause cursor dismissal (overlay 7, 0x0207FE60-0x0207FE90).
 *
 * Marks the main-menu cursor/icon task group and resets its palette pulse.
 */
#include "pause_scene_internal.h"
#include <game/pause_menu_cursor.h>

extern "C" void func_ov005_020663d8(int);

extern "C" void PauseMenuCursor_Close(void)
{
    func_ov005_020663d8(3);
    GamePaletteEffects_ResetEntry(WORK.palette_controller, 0);
}
