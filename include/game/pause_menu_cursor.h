#ifndef PIT_GAME_PAUSE_MENU_CURSOR_H
#define PIT_GAME_PAUSE_MENU_CURSOR_H
#include <nitro.h>
/* Position and icon scale use twelve fractional bits. Cursor velocities use eight. */
typedef struct PauseMenuCursorTask {
    u8 unknown_00[16];
    struct PauseMenuCursorTask *parent;
    u8 unknown_14[8];
    void *resource;
    int state, counter;
    u8 page, immediate;
    u8 unknown_2a[2];
    int x, y;
    union {
        struct {
            s16 offset_x, offset_y, velocity_x, velocity_y, velocity_offset, acceleration_x, acceleration_y,
                acceleration_offset;
            u8 unknown_44[4];
        } motion;
        struct {
            int scale_x, scale_y;
            u8 unknown_3c[12];
        } icon;
    };
} PauseMenuCursorTask;

typedef char PauseMenuCursorTask_SizeCheck[sizeof(PauseMenuCursorTask) == 72 ? 1 : -1];
enum PauseMenuIconState {
    PAUSE_MENU_ICON_INACTIVE,
    PAUSE_MENU_ICON_GROW_Y = 100,
    PAUSE_MENU_ICON_SHRINK_Y,
    PAUSE_MENU_ICON_GROW_X,
    PAUSE_MENU_ICON_SHRINK_X,
    PAUSE_MENU_ICON_PULSE_REVERSE = 200,
    PAUSE_MENU_ICON_PULSE_FORWARD
};
#ifdef __cplusplus
extern "C" {
#endif
void PauseMenuCursor_UpdateIcon(PauseMenuCursorTask *task);
void PauseMenuCursor_UpdateShadow(PauseMenuCursorTask *task);
void PauseMenuCursor_Close(void);
#ifdef __cplusplus
}
#endif
#endif
