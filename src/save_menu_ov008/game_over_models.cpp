/*
 * Game-over models and cursor (overlay 8, 0x02070560-0x0207075C).
 *
 * Creates the menu sprites and positions the cursor at the selected choice.
 */

#include <game/battle_scene.h>
#include <game/save_menu.h>
extern "C" {
#include "save_menu_internal.h"
void *func_ov005_020698dc(int);
BattleModel *Overlay5ResourceA_Attach(void *, BattleModel *, int);
void func_ov005_02068908(BattleModel *, int, void *, int, int);
void Overlay5ResourceA_ApplySelector(SaveMenuTransferTask *);
SaveMenuCursorSpritePrefix *Overlay5ResourceA_Get(void *);
void *func_ov005_02069084(void *, int);
#define WORK (*(SaveMenuCursorWorkPrefix *)data_ov008_02078290)

void GameOverMenu_UpdateCursor(SaveMenuTransferTask *element)
{
    SaveMenuCursorTask *task = (SaveMenuCursorTask *)element;
    SaveMenuCursorSpritePrefix *sprite = Overlay5ResourceA_Get(task);
    switch (task->state) {
    case 0:
        task->selection = 0;
        task->x = WORK.action_x;
        task->y = data_ov008_0207843c[task->selection];
        task->state = 100;
        break;
    case 100:
        task->selection = WORK.menu.selection;
        task->x = WORK.action_x;
        task->y = data_ov008_0207843c[task->selection];
        break;
    }
    {
        int x = task->x / 4096;
        int y = task->y / 4096;
        sprite->x = x;
        sprite->y = y;
    }
    if (WORK.menu.cursor_hidden == 0)
        func_ov005_02069084(sprite, 8);
}

void GameOverMenu_CreateModels(int show_cursor)
{
    void *resource = func_ov005_020698dc(1);
    SaveMenuTransferTask *task = func_ov005_0206659c(Overlay5ResourceA_ApplySelector, 2, 1);
    BattleModel *model;
    task->arguments[0] = 60;
    model = Overlay5ResourceA_Attach(task, 0, 1);
    func_ov005_02068908(model, 0, resource, 0, -1);
    model->set_primary_animation(0, 0, 1);
    model->flag_bits.unknown_00_01 = 2;
    model->animation_offset_x = 48;
    model->animation_offset_y = 24;
    if (show_cursor) {
        resource = func_ov005_020698dc(0);
        task = func_ov005_0206659c(GameOverMenu_UpdateCursor, 2, 1);
        model = Overlay5ResourceA_Attach(task, 0, 1);
        func_ov005_02068908(model, 0, resource, 1, -1);
        model->set_primary_animation(0, 0, 1);
        model->flag_bits.unknown_00_01 = 2;
    }
}
}
