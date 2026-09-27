/* Delayed, per-column fades between the two save-menu location panels. */
#include <game/save_menu.h>

typedef struct SaveMenuPanelFadeTask {
    u8 unknown_00[36];
    int delay;
    int panel;
    int column;
    int opacity;
    int velocity;
    u8 unknown_38[16];
} SaveMenuPanelFadeTask;

typedef char SaveMenuPanelFadeTaskSizeCheck[
    sizeof(SaveMenuPanelFadeTask) == 72 ? 1 : -1];
typedef char SaveMenuPanelFadeTaskOpacityOffsetCheck[
    (u32)&((SaveMenuPanelFadeTask *)0)->opacity == 48 ? 1 : -1];

extern u8 data_ov008_0207aa72[2][14];
void func_ov005_0206650c(SaveMenuTransferTask *task);

void SaveMenuPanel_UpdateFade(SaveMenuTransferTask *base)
{
    SaveMenuPanelFadeTask *task = (SaveMenuPanelFadeTask *)base;
    int panel = task->panel;
    int column = task->column;

    if (task->delay) {
        --task->delay;
        return;
    }

    task->opacity += task->velocity;
    if (task->velocity > 0) {
        if (task->opacity >= 31 * 4096) {
            data_ov008_0207aa72[panel][column] = 31;
            func_ov005_0206650c(base);
            return;
        }
    } else if (task->opacity <= 0) {
        data_ov008_0207aa72[panel][column] = 0;
        func_ov005_0206650c(base);
        return;
    }

    /* The accumulator is Q12; the renderer consumes a five-bit opacity. */
    data_ov008_0207aa72[panel][column] = task->opacity / 4096;
}
