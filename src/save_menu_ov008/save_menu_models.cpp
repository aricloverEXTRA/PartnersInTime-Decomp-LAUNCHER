/*
 * Save menu models (overlay 8, 0x0206BF64-0x0206C078).
 *
 * Creates the 3D models the save menu draws.
 */

#include <game/battle_scene.h>
extern "C" {
#include <game/save_menu.h>
void *func_ov005_020698dc(int);
SaveMenuTransferTask *func_ov005_0206659c(void (*)(SaveMenuTransferTask *), int, int);
void Overlay5ResourceA_ApplySelector(SaveMenuTransferTask *);
BattleModel *Overlay5ResourceA_Attach(SaveMenuTransferTask *, BattleModel *, int);
void func_ov005_02068908(BattleModel *, int, void *, int, int);
void func_ov008_0206c078(SaveMenuTransferTask *);

void SaveMenu_CreateModels(void)
{
    void *resource = func_ov005_020698dc(1);
    SaveMenuTransferTask *task = func_ov005_0206659c(Overlay5ResourceA_ApplySelector, 2, 1);
    BattleModel *model;
    task->arguments[0] = 60;
    model = Overlay5ResourceA_Attach(task, 0, 1);
    func_ov005_02068908(model, 0, resource, 0, -1);
    model->set_primary_animation(1, 0, 1);
    model->flag_bits.unknown_00_01 = 2;
    model->animation_offset_x = 48;
    model->animation_offset_y = 24;
    resource = func_ov005_020698dc(0);
    task = func_ov005_0206659c(func_ov008_0206c078, 2, 1);
    model = Overlay5ResourceA_Attach(task, 0, 1);
    func_ov005_02068908(model, 0, resource, 1, -1);
    model->set_primary_animation(0, 0, 1);
    model->flag_bits.unknown_00_01 = 2;
}
}
