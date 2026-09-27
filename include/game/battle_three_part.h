#ifndef PIT_GAME_BATTLE_THREE_PART_H
#define PIT_GAME_BATTLE_THREE_PART_H

#include <game/battle_scene.h>

#ifdef __cplusplus
extern "C" {
#endif

void BattleThreePart_DrawSubscreen(BattleModel *model,
    const MtxFx44 *transform, int y_offset);

/* Channels are 0..2; the object must own an initialized three-part override. */
int BattleThreePart_GetStatus(BattleSceneObject *object, int channel);
void BattleThreePart_BindActor(BattleSceneObject *object, u16 actor_id, int channel);
void BattleThreePart_SetParameter1D6(BattleSceneObject *object, int value);
void BattleThreePart_SetParameter1D4(BattleSceneObject *object, int value);
void BattleThreePart_PrepareResource(u16 resource_id);
void BattleThreePart_Initialize(BattleSceneObject *object);

#ifdef __cplusplus
}
#endif
#endif
