/*
 * Save-menu cursor and motion (overlay 8, 0x0206C078-0x0206C378).
 *
 * Move between the three save choices and the confirmation prompt.
 * Q12 positions divide toward zero before conversion to sprite coordinates.
 */

#include "save_menu_internal.h"

void SaveMenuMotion_Initialize(SaveMenuMotion *motion, int x, int y)
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

int SaveMenuMotion_Update(SaveMenuMotion *motion)
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
#define WORK (*(SaveMenuCursorWorkPrefix *)data_ov008_02078290)

void SaveMenu_UpdateCursor(SaveMenuTransferTask *element)
{
    SaveMenuCursorTask *cursor = (SaveMenuCursorTask *)element;
    SaveMenuCursorSpritePrefix *sprite = Overlay5ResourceA_Get(cursor);
    switch (cursor->state) {
    case 0:
        cursor->selection = 0;
        cursor->x = WORK.action_x;
        cursor->y = data_ov008_0207843c[cursor->selection];
        cursor->state = 100;
        break;
    case 10:
        if (!SaveMenuMotion_Update((SaveMenuMotion *)cursor)) break;
        cursor->state = 100;
        /* Handle the restored choice on the arrival frame. */
    case 100:
        if (WORK.menu.menu_mode == 1) {
            int y = WORK.confirmation_y;
            int x = WORK.confirmation_x;
            cursor->velocity_x = 0;
            cursor->velocity_y = -32768;
            cursor->counter = 8;
            SaveMenuMotion_Initialize((SaveMenuMotion *)cursor, x, y + 65536);
            ++cursor->state;
        } else {
            cursor->selection = WORK.menu.selection;
            cursor->x = WORK.action_x;
            cursor->y = data_ov008_0207843c[cursor->selection];
        }
        break;
    case 101:
        if (!SaveMenuMotion_Update((SaveMenuMotion *)cursor)) break;
        cursor->state = 200;
        /* Position the confirmation cursor on the arrival frame. */
    case 200:
        if (!WORK.menu.menu_mode) {
            int x = WORK.action_x;
            int y = data_ov008_0207843c[WORK.menu.previous_selection];
            cursor->velocity_x = 0;
            cursor->velocity_y = -32768;
            cursor->counter = 8;
            SaveMenuMotion_Initialize((SaveMenuMotion *)cursor, x, y);
            cursor->state = 10;
        } else {
            cursor->selection = WORK.menu.selection;
            cursor->x = WORK.confirmation_x;
            cursor->y = WORK.confirmation_y + (cursor->selection << 16);
        }
        break;
    }
    {
        int x = cursor->x / 4096;
        int y = cursor->y / 4096;
        sprite->x = x;
        sprite->y = y;
    }
    if (!WORK.menu.cursor_hidden)
        func_ov005_02069084(sprite, 8);
}
