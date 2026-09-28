/*
 * Request exit motion for the shared equipment and status display tasks.
 * Overlay 7, 0x02079D08-0x02079D1C.
 */
#include "pause_scene_internal.h"
#include <game/menu_equipment.h>
extern "C" void MenuEquipment_RequestClose(void)
{
    WORK.equipment_active = 0;
}
