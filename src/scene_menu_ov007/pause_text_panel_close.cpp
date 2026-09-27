/*
 * Pause text-panel close request (overlay 7, 0x0207B2C8-0x0207B2DC).
 *
 * The panel updater consumes this shared flag and starts its exit motion.
 */
#include "pause_scene_internal.h"
#include <game/pause_text_panel.h>

extern "C" void PauseTextPanel_RequestClose(void)
{
    WORK.text_panel_active = 0;
}
