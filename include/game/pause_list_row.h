#ifndef PIT_GAME_PAUSE_LIST_ROW_H
#define PIT_GAME_PAUSE_LIST_ROW_H

#include <nitro.h>
#include <nitro/fx.h>
#include <game/pause_scene.h>

/* The parent task supplies a list row's text tiles, quantity and position. */
typedef struct PauseListRowTask {
    u8 unknown_00[40];
    u16 item, row, quantity, palette_offset, draw_list;
    u8 unknown_32[2];
    int tile;
    fx32 x, y, origin_x, origin_y;
} PauseListRowTask;

typedef struct PauseListSpriteTask {
    u8 unknown_00[16];
    PauseListRowTask *parent;
    u8 unknown_14[20];
    union { int segment; int divisor; } selection;
    u8 unknown_2c[28];
} PauseListSpriteTask;

/* One equipped-item indicator for a displayed party member. */
typedef struct PauseEquippedMarkerTask {
    u8 unknown_00[40];
    int member;
    u8 unknown_2c[28];
} PauseEquippedMarkerTask;

/* A queued text/marker upload; the callback marks it for later removal. */
typedef struct PauseItemRowDrawTask {
    u8 unknown_00[40];
    int row, item;
    u8 unknown_30[24];
} PauseItemRowDrawTask;

typedef char PauseItemRowDrawTaskSizeCheck[sizeof(PauseItemRowDrawTask) == 72 ? 1 : -1];
typedef char PauseListRowTaskSizeCheck[sizeof(PauseListRowTask) == 72 ? 1 : -1];
typedef char PauseListSpriteTaskSizeCheck[sizeof(PauseListSpriteTask) == 72 ? 1 : -1];
typedef char PauseEquippedMarkerTaskSizeCheck[sizeof(PauseEquippedMarkerTask) == 72 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
PauseMenuElement *PauseList_QueueRowDraw(int row, int item);
void PauseList_DrawRowTask(PauseMenuElement *element);
void PauseListRow_Refresh(PauseListRowTask *task);
void PauseListRow_UpdateDigitSprite(PauseListSpriteTask *task);
void PauseListRow_UpdateMarkerSprite(PauseListSpriteTask *task);
void PauseListRow_UpdateTextSprite(PauseListSpriteTask *task);
void PauseEquippedMarker_Update(PauseEquippedMarkerTask *task);
void PauseList_CreateEmptyRowSprite(void);
void PauseList_UpdateEmptyRowSprite(PauseMenuElement *task);
void PauseList_UpdateBeanDigit(PauseMenuElement *task);
#ifdef __cplusplus
}
#endif

#endif
