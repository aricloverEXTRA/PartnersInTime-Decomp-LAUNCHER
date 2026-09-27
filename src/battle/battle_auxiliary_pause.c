/* Pause control for auxiliary battle objects (overlay 2, 0x02076C38-0x02076C74). */

#include <game/battle_scene.h>
#include <game/battle_script_properties.h>

void BattleScene_SetAuxiliaryObjectsPaused(int paused)
{
    /* Objects 40..55 are the auxiliary scene slots below the party IDs. */
    int id;
    if (paused)
        paused = 1;
    for (id = 40; id < 56; ++id)
        BattleScript_SetProperty((u16)id, BATTLE_PROPERTY_SCENE_1A, paused);
}
