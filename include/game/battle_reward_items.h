#ifndef PIT_GAME_BATTLE_REWARD_ITEMS_H
#define PIT_GAME_BATTLE_REWARD_ITEMS_H

/*
 * The reward item display shown after a victory.
 */
#ifdef __cplusplus
extern "C" {
#endif
void BattleRewardItems_Initialize(void);
void BattleRewardItems_Update(void);
void BattleRewardItems_BeginClose(void);
int BattleRewardItems_IsReady(void);
int BattleRewardItems_IsDone(void);
#ifdef __cplusplus
}
#endif
#endif
