/* Battle window close, overlay 2, 0x02070558-0x020705E0. */

#include <game/battle_window_upload.h>
#include <game/battle_scene.h>
extern "C" {
void func_0202cbd4(void *, int, u32);
void BattleWindow_Close(BattleWindowManager *manager, s16 index)
{
    if (manager->base.windows[index].properties.shape.bits.screen == 1)
        ((BattleWindowUploadFlags *)&manager->flags)->bits.pending = 1;
    /* Detach before clearing the state: the global motion list owns its link. */
    BattleSceneObject_UnlinkMotion((BattleSceneObject *)manager->windows[index].motion_state);
    func_0202cbd4(&manager->windows[index], 0, sizeof(BattleWindowState));
    GameWindow_Close(&manager->base, index);
}
}
