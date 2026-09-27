/*
 * Pause main menu (overlay 7, 0x0206F240-0x0206F754).
 *
 * The main pause menu's per-frame task and the position each party member's
 * entry is drawn at.
 */

#include "pause_scene_internal.h"
#include <game/pause_text_panel.h>
#include <game/pause_navigation.h>
#include <game/pause_background.h>
#include <game/pause_menu_label.h>

extern "C" {
extern const int data_ov007_0208d964[2][2];
void func_ov007_0207b2dc(int, int);
void func_ov007_0206e27c(PauseMenuElement *);
void func_ov005_02066358(PauseMenuElement *, void (*)(PauseMenuElement *), int);
void func_ov005_0206650c(void *);
void GameResource_Move16(const void *, void *, u32);
}

/* Preserve the two input-cache halfwords and their separate native reads. */
#define NAV_INPUT(slot) (((u16 *)WORK.unknown00)[slot])

extern "C" void PauseMenu_UpdateTask(PauseMainMenuTask *task)
{
    switch (task->phase) {
    case 0:
        PauseMenuLabel_DrawPage((u8)WORK.selected_menu);
        func_ov007_0207b2dc(1, 0);
        task->selection_delay = 0;
        task->queued_action = 0;
        task->queued_movement = 0;
        task->phase = 100;
        break;
    case 100:
        if (task->selection_delay) {
            if (NAV_INPUT(2) & 0x80a) task->queued_action = -1;
            if (NAV_INPUT(2) & 0x401) task->queued_action = 1;
            if (NAV_INPUT(1) & 0x40) {
                task->queued_movement = -1;
                if (NAV_INPUT(2) & 0x40) task->queued_movement *= 2;
            }
            if (NAV_INPUT(1) & 0x80) {
                task->queued_movement = 1;
                if (NAV_INPUT(2) & 0x80) task->queued_movement *= 2;
            }
            --task->selection_delay;
            return;
        }
        WORK.initialized = 0;
        if ((NAV_INPUT(2) & 0x401) || task->queued_action == 1) {
            if (WORK.selected_menu == 5) {
                task->phase = 1000;
                return;
            }
            WORK.initialized = 1;
            task->queued_action = 0;
            task->queued_movement = 0;
            PauseTextPanel_RequestClose();
            PauseBackground_Upload(0);
            PauseBackground_StartPageLoad((PauseBackgroundTask *)task);
            GamePaletteEffects_ResetEntry(WORK.palette_controller, 0);
            ++task->phase;
        } else if ((NAV_INPUT(2) & 0x80a) || task->queued_action == -1) {
            WORK.initialized = 1;
            task->queued_action = 0;
            task->queued_movement = 0;
            PauseTextPanel_RequestClose();
            data_ov007_0208e1e0->phase = 5;
            func_ov005_02066358((PauseMenuElement *)task, func_ov007_0206e27c, 0);
        } else {
            int previous = WORK.selected_menu;
            if ((NAV_INPUT(1) & 0x40) || task->queued_movement < 0) --WORK.selected_menu;
            if ((NAV_INPUT(1) & 0x80) || task->queued_movement > 0) ++WORK.selected_menu;
            if (WORK.selected_menu < 0) {
                if ((NAV_INPUT(2) & 0x40) || task->queued_movement < -1)
                    WORK.selected_menu = WORK.menu_count - 1;
                else WORK.selected_menu = 0;
            }
            if (WORK.selected_menu >= WORK.menu_count) {
                if ((NAV_INPUT(2) & 0x80) || task->queued_movement > 1)
                    WORK.selected_menu = 0;
                else WORK.selected_menu = WORK.menu_count - 1;
            }
            task->queued_movement = 0;
            if (previous != WORK.selected_menu) {
                PauseMenuLabel_DrawPage((u8)WORK.selected_menu);
                task->selection_delay = 5;
                task->queued_action = 0;
            }
        }
        break;
    case 101:
        if (!task->background_pending) {
            const void *source = (u8 *)Overlay5Display_GetObjPalette(DISPLAY_ENGINE_MAIN) + 384;
            void *destination = (u8 *)Overlay5Display_GetObjPalette(DISPLAY_ENGINE_SUB) + 384;
            GameResource_Move16(source, destination, 32);
            data_ov007_0208e1e0->phase = 3;
            /* This setter immediately runs the new page callback. */
            func_ov005_02066358((PauseMenuElement *)task,
                (void (*)(PauseMenuElement *))PausePage_OpenTask, 1);
        }
        break;
    case 1000:
        ((u8 *)gSaveData)[0x55d] = WORK.selected_menu + 1;
        task->brightness = 0;
        ++task->phase;
        /* fall through */
    case 1001:
        ++task->brightness;
        func_ov005_0206781c(2, -task->brightness);
        if (task->brightness == 16) {
            data_ov007_0208e1e0->phase = 7;
            func_ov005_0206650c(task);
        }
        break;
    }
}

extern "C" void PauseMenu_GetMemberPosition(int member, int *x, int *y)
{
    for (int row = 0; row < 2; ++row) {
        for (int column = 0; column < 2; ++column) {
            if (member == data_ov007_0208d964[row][column]) {
                *x = column;
                *y = row;
                return;
            }
        }
    }
    *x = 0;
    *y = 0;
}
