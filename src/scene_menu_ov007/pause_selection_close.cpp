/*
 * Item-selection cleanup: mark its task group and release the shared sub-screen sprite tiles.
 * Overlay 7, 0x0207B864-0x0207B88C.
 */
#include "pause_scene_internal.h"
#include <game/pause_selection_sprites.h>
extern "C" void func_ov005_020663d8(int);
extern "C" void PauseEquipment_CloseItemSelection(void)
{
    func_ov005_020663d8(3);
    GameSpriteAllocation_Unlink(&WORK.sub_allocation);
}
