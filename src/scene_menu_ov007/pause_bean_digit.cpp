/*
 * Bean count digit visibility (overlay 7, 0x02073708-0x02073754).
 *
 * Digit sprites are submitted only while the selected key-item row is beans.
 */
#include "pause_scene_internal.h"
#include <game/pause_list_row.h>
#include <game/overlay005_resource.h>
#include <game/overlay007_party.h>

extern "C" {
void *Overlay5ResourceB_Get(PauseMenuElement *);
void func_ov005_02069084(void *, int);
u8 func_ov007_02075408(Overlay7Party *);
u8 func_ov007_02075324(Overlay7Party *, int);
}
#define PARTY ((Overlay7Party *)data_ov007_0208e1e4)

extern "C" void PauseList_UpdateBeanDigit(PauseMenuElement *task)
{
    Overlay5ObjectSprite *sprite = (Overlay5ObjectSprite *)Overlay5ResourceB_Get(task);
    int row = func_ov007_02075408(PARTY);
    if (!func_ov007_02075324(PARTY, row))
        func_ov005_02069084(sprite, 41);
}
