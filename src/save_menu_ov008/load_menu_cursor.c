/*
 * Load-menu cursor and motion (overlay 8, 0x0206EE58-0x0206F48C).
 *
 * Move between save panels, the action menu and both confirmation prompts.
 * Coordinates and motion are Q12; draw coordinates truncate toward zero.
 */
#include "save_menu_internal.h"

void LoadMenuMotion_Initialize(SaveMenuMotion *motion, int x, int y)
{
    int acceleration_y;
    u8 *menu;
    int dy = y - motion->y;

    motion->acceleration_x = 2 * (x - motion->x - motion->velocity_x * motion->remaining)
        / (motion->remaining * motion->remaining);
    acceleration_y = 2 * (dy - motion->velocity_y * motion->remaining)
        / (motion->remaining * motion->remaining);
    menu = data_ov008_02078290;
    motion->acceleration_y = acceleration_y;
    menu[412] = 1;
}

int LoadMenuMotion_Update(SaveMenuMotion *motion)
{
    motion->velocity_x += motion->acceleration_x;
    motion->velocity_y += motion->acceleration_y;
    motion->x += motion->velocity_x;
    motion->y += motion->velocity_y;
    if (--motion->remaining == 1) {
        data_ov008_02078290[412] = 0;
        return 1;
    }
    return 0;
}

extern SaveMenuCursorSpritePrefix *Overlay5ResourceA_Get(void *);
extern void *func_ov005_02069084(void *, int);
/* Preserve the native coordinate reads before changing the motion fields.
 * The two aliases address interleaved unsigned X/Y bytes with stride two. */
extern volatile const u8 data_ov008_02077f30[], data_ov008_02077f31[];
#define WORK (*(SaveMenuCursorWorkPrefix *)data_ov008_02078290)
#define MOTION ((SaveMenuMotion *)task)

void LoadMenu_UpdateCursor(SaveMenuTransferTask *element)
{
    SaveMenuCursorTask *task = (SaveMenuCursorTask *)element;
    SaveMenuCursorSpritePrefix *sprite = Overlay5ResourceA_Get(task);
    switch (task->state) {
    case 0:
        task->selection = WORK.menu.selected_panel;
        task->x = (data_ov008_02077f30[2 * task->selection] + 8) << 12;
        task->y = (data_ov008_02077f31[2 * task->selection] + 16) << 12;
        task->state = 100;
        break;
    case 100:
        if (WORK.menu.menu_mode == 1) {
            int x = WORK.action_x;
            int y = WORK.action_y;
            WORK.menu.selection = 0;
            task->velocity_x = 0;
            task->velocity_y = -32768;
            task->counter = 8;
            LoadMenuMotion_Initialize(MOTION, x, y);
            task->state = 110;
        } else if (task->selection != WORK.menu.selected_panel) {
            int x = data_ov008_02077f30[2 * WORK.menu.selected_panel] + 8;
            int y = data_ov008_02077f31[2 * WORK.menu.selected_panel] + 16;
            task->velocity_x = -16384;
            task->velocity_y = 0;
            task->counter = 8;
            LoadMenuMotion_Initialize(MOTION, x << 12, y << 12);
            ++task->state;
        }
        break;
    case 101:
        if (LoadMenuMotion_Update(MOTION)) {
            task->selection = WORK.menu.selected_panel;
            task->x = (data_ov008_02077f30[2 * task->selection] + 8) << 12;
            task->y = (data_ov008_02077f31[2 * task->selection] + 16) << 12;
            --task->state;
        }
        break;
    case 110:
        if (!LoadMenuMotion_Update(MOTION)) break;
        task->state = 200;
        /* The first stationary update runs on the arrival frame. */
    case 200:
        if (WORK.menu.menu_mode == 0) {
            int x = data_ov008_02077f30[2 * WORK.menu.selected_panel] + 8;
            int y = data_ov008_02077f31[2 * WORK.menu.selected_panel] + 16;
            task->velocity_x = 0;
            task->velocity_y = -32768;
            task->counter = 8;
            LoadMenuMotion_Initialize(MOTION, x << 12, y << 12);
            task->state = 101;
        } else if ((u8)(WORK.menu.menu_mode + 254) <= 1) {
            /* Unsigned-byte range test for copy (2) or the first delete prompt (3). */
            int y = WORK.confirmation_y;
            int x = WORK.confirmation_x;
            task->velocity_x = 0;
            task->velocity_y = -32768;
            task->counter = 8;
            LoadMenuMotion_Initialize(MOTION, x, y + 65536);
            ++task->state;
        } else {
            task->selection = WORK.menu.selection;
            task->x = WORK.action_x;
            task->y = WORK.action_y + ((18 * task->selection) << 12);
        }
        break;
    case 201:
        if (!LoadMenuMotion_Update(MOTION)) break;
        task->state = 300;
        /* Fall through to the first confirmation cursor. */
    case 300:
        if (WORK.menu.menu_mode == 1) {
            int y = WORK.action_y + ((18 * WORK.menu.previous_selection) << 12);
            int x = WORK.action_x;
            task->velocity_x = 0;
            task->velocity_y = -32768;
            task->counter = 8;
            LoadMenuMotion_Initialize(MOTION, x, y);
            task->state = 110;
        } else if (WORK.menu.menu_mode == 4) {
            int y = WORK.confirmation_y;
            int x = WORK.confirmation_x;
            task->velocity_x = 0;
            task->velocity_y = -32768;
            task->counter = 8;
            LoadMenuMotion_Initialize(MOTION, x, y + 65536);
            ++task->state;
        } else {
            task->selection = WORK.menu.selection;
            task->x = WORK.confirmation_x;
            task->y = WORK.confirmation_y + (task->selection << 16);
        }
        break;
    case 301:
        if (!LoadMenuMotion_Update(MOTION)) break;
        task->state = 400;
        /* Fall through to the second confirmation cursor. */
    case 400:
        if (WORK.menu.menu_mode == 0) {
            int x = data_ov008_02077f30[2 * WORK.menu.selected_panel] + 8;
            int y = data_ov008_02077f31[2 * WORK.menu.selected_panel] + 16;
            task->velocity_x = 0;
            task->velocity_y = -32768;
            task->counter = 8;
            LoadMenuMotion_Initialize(MOTION, x << 12, y << 12);
            task->state = 101;
        } else if (WORK.menu.menu_mode == 1) {
            int y = WORK.action_y + ((18 * WORK.menu.previous_selection) << 12);
            int x = WORK.action_x;
            task->velocity_x = 0;
            task->velocity_y = -32768;
            task->counter = 8;
            LoadMenuMotion_Initialize(MOTION, x, y);
            task->state = 110;
        } else {
            task->selection = WORK.menu.selection;
            task->x = WORK.confirmation_x;
            task->y = WORK.confirmation_y + (task->selection << 16);
        }
        break;
    }
    {
        int x = task->x / 4096;
        int y = task->y / 4096;
        sprite->x = x;
        sprite->y = y;
    }
    if (!WORK.menu.cursor_hidden)
        func_ov005_02069084(sprite, 8);
}
