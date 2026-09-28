/* Copy confirmation and asynchronous write results in the load menu. */
#include "save_write_effects_internal.h"
#include <game/save_menu_write.h>
#include <game/save_state_transfer.h>
#include <nitro/card.h>

extern void func_ov005_02066358(MenuElement *, void (*)(MenuElement *), int);
extern void func_ov008_0206e310(MenuElement *);
extern void func_ov008_0206ec9c(int);
#define WORK (*(SaveMenuEntryWorkPrefix *)data_ov008_02078290)
#define BUTTONS (*(GameInput *)data_0206032c)
#define LIVE_SAVE ((SaveLiveTransferView *)gSaveData)

void LoadMenu_UpdateCopyConfirmation(SaveMenuConfirmTask *task)
{
    switch (task->state) {
    case 0:
        WORK.selection = 1;
        WORK.menu_mode = 2;
        SaveMenuText_BuildDialog(data_ov008_0207828c, 3, 14);
        task->counter = 0;
        task->state = 100;
        break;
    case 100:
        if (!WORK.input_locked) {
            u16 pressed = BUTTONS.pressed;
            int action = 0;
            if (pressed & 0x401) {
                if (!WORK.selection) action = 1;
                else action = -1;
            }
            if (pressed & 0x802) action = -1;
            if (action == 1) {
                func_ov005_02069bcc(232, 0, 0, 128);
                if (!SaveStorage_Probe()) {
                    data_02059f44 = 1;
                    LIVE_SAVE->slot_select.bits.unknown_5 = 0;
                    WORK.scroll_locked = 1;
                    task->state = 1000;
                } else {
                    SaveStorageSlot *source = SaveStorage_GetSlot((u8)WORK.selected_panel);
                    SaveStorageSlot *destination = SaveStorage_GetSlot((u8)WORK.copy_destination);
                    MI_CpuCopy8(source, destination, sizeof(SaveStorageSlot));
                    SaveMenuWrite_Start((SaveMenuWriteTask *)task,
                        (u8)WORK.selected_panel, (u8)WORK.copy_destination, 1, 0);
                    SaveMenuMessage_Show(data_ov008_0207828c, 16);
                    task->counter = 60;
                    task->state = 200;
                }
            } else if (action == -1) {
                func_ov005_02069bcc(3, 0, 0, 128);
                func_ov005_02066358((MenuElement *)task, func_ov008_0206e310, 0);
            } else {
                int initial_selection = WORK.selection;
                u8 previous = initial_selection;
                if (pressed & 0x40) WORK.selection = initial_selection - 1;
                if (pressed & 0x80) ++WORK.selection;
                if (WORK.selection < 0) WORK.selection = 1;
                if (WORK.selection > 1) WORK.selection = 0;
                if (previous != WORK.selection) func_ov005_02069bcc(231, 0, 4, 128);
            }
        }
        break;
    case 200:
        if (task->counter) {
            --task->counter;
        } else {
            switch (task->result) {
            case SAVE_WRITE_OK: {
                /* Match the word-fill wrapper's volatile stack temporary. */
                volatile u32 clear;
                MI_CpuCopy8(&WORK.summaries[WORK.selected_panel],
                    &WORK.summaries[WORK.copy_destination], sizeof(SaveMenuSummary));
                clear = 0;
                func_0203b7a0(clear, data_ov008_0207828c->pixels, sizeof(data_ov008_0207828c->pixels));
                SaveMenu_CopyLocationPanel((u8)WORK.copy_destination, (u8)WORK.selected_panel);
                task->state = 300;
                break;
            }
            case SAVE_WRITE_SLOT_ERROR:
                func_ov008_0206ec9c(WORK.copy_destination);
                SaveMenuMessage_Show(data_ov008_0207828c, 17);
                task->counter = 30;
                ++task->state;
                break;
            case SAVE_WRITE_SETTINGS_ERROR:
                SaveMenuMessage_Show(data_ov008_0207828c, 32);
                task->counter = 30;
                ++task->state;
                break;
            }
        }
        break;
    case 201:
        if (task->counter) {
            --task->counter;
            break;
        }
        if (!(BUTTONS.pressed & 0x401)) break;
        func_ov005_02069bcc(232, 0, 0, 128);
        /* The acknowledgement shares cleanup with successful copying. */
    case 300:
        SaveMenuMessage_Hide(data_ov008_0207828c);
        WORK.previous_selection = 0;
        func_ov005_02066358((MenuElement *)task, func_ov008_0206e310, 0);
        break;
    case 1000:
        SaveMenuMessage_Show(data_ov008_0207828c, 28);
        ++task->state;
        break;
    }
}
