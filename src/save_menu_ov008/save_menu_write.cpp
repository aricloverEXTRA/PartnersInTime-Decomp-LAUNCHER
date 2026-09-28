extern "C" {

/*
 * Save writing (overlay 8, 0x02073F04-0x0207431C).
 *
 * Starts the write to the battery file and steps it, showing and hiding the
 * message that warns the player not to switch off.
 */

#include <game/save_menu_write.h>
#include <game/save_storage.h>
#include <game/save_state_transfer.h>
#include <game/overlay005_display.h>
extern u8 data_02059f44, data_02059f48;
extern SaveMenuSlotSettings data_0205e334;
extern SaveMenuBufferHeader data_0205e32c;
extern u8 data_ov008_02078290[];
SaveMenuWriteTask *func_ov005_0206659c(void (*)(SaveMenuWriteTask *), int, int);
void func_ov005_0206650c(SaveMenuWriteTask *);
}
#define LIVE ((SaveLiveTransferView *)gSaveData)
enum { SAVE_MENU_MESSAGE_VISIBLE_OFFSET = 411 };

extern "C" void SaveMenuMessage_Hide(SaveMenuText *text)
{
    data_ov008_02078290[SAVE_MENU_MESSAGE_VISIBLE_OFFSET] = 0;
    Overlay5Display_DisablePlanes(DISPLAY_ENGINE_MAIN, DISPLAY_PLANE_BG1);
    Overlay5Display_DisablePlanes(DISPLAY_ENGINE_MAIN, DISPLAY_PLANE_BG2);
}
extern "C" void SaveMenuMessage_Show(SaveMenuText *text, u16 entry)
{
    SaveMenuMessage_Hide(text);
    SaveMenuText_DrawBackground(text, 0, 2, 0, entry, 0, 0);
    data_ov008_02078290[SAVE_MENU_MESSAGE_VISIBLE_OFFSET] = 1;
    Overlay5Display_EnablePlanes(DISPLAY_ENGINE_MAIN, DISPLAY_PLANE_BG1);
    Overlay5Display_EnablePlanes(DISPLAY_ENGINE_MAIN, DISPLAY_PLANE_BG2);
}

extern "C" void SaveMenuWrite_Update(SaveMenuWriteTask *task)
{
    int state = task->state;
    u8 source = task->source_slot, destination = task->destination_slot, checksum = task->checksum;
    s8 selection = task->selection_mode;
    switch (state) {
    case SAVE_WRITE_START:
        task->errors = 0;
        task->state = SAVE_WRITE_PRIMARY;
        /* The first invocation starts the write immediately. */
    case SAVE_WRITE_PRIMARY:
        if (SaveStorage_WriteSlot(source, destination, checksum))
            task->state = SAVE_WRITE_WAIT_PRIMARY;
        else {
            task->errors |= SAVE_WRITE_PRIMARY_ERROR;
            task->state = SAVE_WRITE_COPY;
        }
        break;
    case SAVE_WRITE_WAIT_PRIMARY: {
        u8 result = SaveStorage_GetState();
        if (result == 1)
            break;
        if (result == 2) {
            task->errors |= SAVE_WRITE_PRIMARY_ERROR;
            task->state = SAVE_WRITE_COPY;
        } else if (result == 0)
            task->state = SAVE_WRITE_COPY;
    } break;
    case SAVE_WRITE_COPY:
        if (SaveStorage_WriteSlot(source, (u8)(destination + 2), checksum))
            task->state = SAVE_WRITE_WAIT_COPY;
        else {
            task->errors |= SAVE_WRITE_COPY_ERROR;
            task->state = SAVE_WRITE_SETTINGS;
        }
        break;
    case SAVE_WRITE_WAIT_COPY: {
        u8 result = SaveStorage_GetState();
        if (result == 1)
            break;
        if (result == 2) {
            task->errors |= SAVE_WRITE_COPY_ERROR;
            task->state = SAVE_WRITE_SETTINGS;
        } else if (result == 0)
            task->state = SAVE_WRITE_SETTINGS;
    } break;
    case SAVE_WRITE_SETTINGS:
        if (task->errors & SAVE_WRITE_PRIMARY_ERROR)
            task->state = SAVE_WRITE_PRIMARY_FAILED;
        else {
            if (selection >= 0)
                data_0205e334.occupied_slots |= 1 << destination;
            else
                data_0205e334.occupied_slots &= ~(1 << destination);
            if (selection > 0)
                data_0205e32c.settings.selected_slot = destination;
            if (!SaveStorage_WriteSettings())
                task->state = SAVE_WRITE_SETTINGS_FAILED;
            else {
                task->result = SAVE_WRITE_OK;
                task->state = SAVE_WRITE_FINISH;
            }
        }
        break;
    case SAVE_WRITE_PRIMARY_FAILED:
        task->result = SAVE_WRITE_SLOT_ERROR;
        task->state = SAVE_WRITE_FINISH;
        break;
    case SAVE_WRITE_SETTINGS_FAILED:
        task->result = SAVE_WRITE_SETTINGS_ERROR;
        task->state = SAVE_WRITE_FINISH;
        break;
    case SAVE_WRITE_FINISH:
        data_02059f44 = task->previous_lock;
        if (gSaveData)
            LIVE->slot_select.bits.unknown_5 = task->previous_bit5;
        data_02059f48 = 0;
        task->parent->child_result = task->result;
        func_ov005_0206650c(task);
        break;
    }
}
extern "C" int SaveMenuWrite_Start(SaveMenuWriteTask *parent, u8 source, u8 destination, u8 checksum,
                                   s8 selection)
{
    SaveMenuWriteTask *task = func_ov005_0206659c(SaveMenuWrite_Update, 2, 1);
    task->source_slot = source;
    task->destination_slot = destination;
    task->checksum = checksum;
    task->selection_mode = selection;
    task->previous_lock = data_02059f44;
    data_02059f44 = 1;
    data_02059f48 = 1;
    if (gSaveData) {
        task->previous_bit5 = LIVE->slot_select.bits.unknown_5 != 0;
        LIVE->slot_select.bits.unknown_5 = 0;
    }
    task->parent = parent;
    parent->child_result = SAVE_WRITE_PENDING;
    return 0;
}
