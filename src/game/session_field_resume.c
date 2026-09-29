/* Resume field execution after a load or retry (ARM9, 0x02028EF0-0x02028F0C). */
#include <game/session.h>
extern GameSessionTask *data_02059ffc;
void GameSession_ResumeField(void) {
    GameSessionTask_RequestStatePhase2(data_02059ffc, 0);
}
