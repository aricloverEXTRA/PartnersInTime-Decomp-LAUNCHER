#ifndef PIT_GAME_PAUSE_SELECTION_SPRITES_H
#define PIT_GAME_PAUSE_SELECTION_SPRITES_H
#include <game/pause_scene.h>
#include <game/pause_navigation.h>
#include <game/menu_equipment.h>

/* Category cursor origin in pixels; the selected category adds a row offset. */
typedef struct PauseSelectionSpriteTask {
    u8 unknown_00[40];
    int x, y;
    u8 unknown_30[24];
} PauseSelectionSpriteTask;
typedef char PauseSelectionSpriteTaskSize[sizeof(PauseSelectionSpriteTask) == 72 ? 1 : -1];

/* Scroll-arrow coordinates are Q12; part 0 is the upper arrow, 1 the lower. */
typedef struct PauseListArrowTask {
    u8 unknown_00[16];
    PausePageTask *parent;
    u8 unknown_14[20];
    int x, y, part;
    u8 unknown_34[20];
} PauseListArrowTask;
typedef char PauseListArrowTaskSize[sizeof(PauseListArrowTask) == 72 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
void PauseEquipment_CloseItemSelection(void);
void PauseList_UpdateItemHeading(MenuEquipmentTask *task);
void PauseList_UpdateScrollArrow(PauseListArrowTask *task);
void PauseList_CreateSelectionSprites(PausePageTask *parent, int suppress_heading);
void PauseEquipment_CreateCategoryCursor(void);
void PauseEquipment_UpdateCategoryCursor(PauseSelectionSpriteTask *task);
void PauseList_UpdateSelectionMarker(PauseMenuElement *task);
void PauseList_UpdateSelectionCursor(PauseMenuElement *task);
#ifdef __cplusplus
}
#endif
#endif
