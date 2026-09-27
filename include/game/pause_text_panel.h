#ifndef PIT_GAME_PAUSE_TEXT_PANEL_H
#define PIT_GAME_PAUSE_TEXT_PANEL_H

#include <nitro.h>

/* A 72-byte overlay-5 task. Position, velocity and acceleration use Q12;
 * targets are signed pixel coordinates. Strip tasks use the parent link. */
typedef struct PauseTextPanelTask {
    u8 unknown_00[16];
    struct PauseTextPanelTask *parent;
    u8 unknown_14[12];
    int phase, timer;
    u16 upper, closing;
    int x, y;
    s16 target_x, target_y;
    int velocity_x, velocity_y, acceleration_x, acceleration_y;
} PauseTextPanelTask;

typedef char PauseTextPanelTask_SizeCheck[sizeof(PauseTextPanelTask) == 72 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
void PauseTextPanel_Update(PauseTextPanelTask *task);
void PauseTextPanel_UpdateStrip(PauseTextPanelTask *task);
#ifdef __cplusplus
}
#endif

#endif
