#ifndef PIT_GAME_SESSION_H
#define PIT_GAME_SESSION_H

/*
 * The running game session: the state it is in and the requests that move it.
 */
#include <game/task.h>

typedef struct GameSessionSettings {
    u16 unknown_0 : 1, unknown_1 : 1, option_2 : 1, option_3 : 1;
    u16 language : 5, unknown_9 : 1, unknown_10_15 : 6;
    s16 unknown_02, unknown_04;
    /* Alignment padding at +6 is not copied by the settings assignment. */
    u32 unknown_08[2];
} GameSessionSettings;

typedef struct GameSessionTask {
    GameTask base;
    u8 unknown_18[16];
    u8 requested_state;
    u8 unknown_29[3];
} GameSessionTask;

typedef char GameSessionSettings_SizeCheck[sizeof(GameSessionSettings) == 16 ? 1 : -1];
typedef char GameSessionTask_SizeCheck[sizeof(GameSessionTask) == 44 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
void GameSession_ResetSaveState(void);
void GameSession_InitializeSaveContext(const GameSessionSettings *settings);
void GameSession_InitSettings(int language, u8 option_2, u8 option_3, GameSessionSettings *settings);
void GameSession_Start(int language, u8 option_2, u8 option_3);
void GameSessionTask_RequestState(GameSessionTask *task, u32 requested_state);
void GameSessionTask_RequestStatePhase2(GameSessionTask *task, u32 requested_state);
void GameSessionTask_Update(GameSessionTask *task);
GameSessionTask *GameSessionTask_Init(GameSessionTask *task, u32 priority, u32 unused,
    void *argument, u32 requested_state, u32 unknown, const GameSessionSettings *settings);
GameSessionTask *GameSessionTask_Destroy(GameSessionTask *task);
GameSessionTask *GameSessionTask_Delete(GameSessionTask *task);
#ifdef __cplusplus
}
#endif

#endif
