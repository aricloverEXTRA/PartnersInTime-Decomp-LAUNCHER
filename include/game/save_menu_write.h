#ifndef PIT_GAME_SAVE_MENU_WRITE_H
#define PIT_GAME_SAVE_MENU_WRITE_H
#include <game/save_menu.h>

enum SaveMenuWriteState {
    SAVE_WRITE_START,
    SAVE_WRITE_PRIMARY,
    SAVE_WRITE_WAIT_PRIMARY,
    SAVE_WRITE_COPY,
    SAVE_WRITE_WAIT_COPY,
    SAVE_WRITE_SETTINGS,
    SAVE_WRITE_PRIMARY_FAILED,
    SAVE_WRITE_SETTINGS_FAILED,
    SAVE_WRITE_FINISH
};
enum SaveMenuWriteResult {
    SAVE_WRITE_PENDING,
    SAVE_WRITE_OK,
    SAVE_WRITE_SLOT_ERROR,
    SAVE_WRITE_SETTINGS_ERROR
};
enum { SAVE_WRITE_PRIMARY_ERROR = 1, SAVE_WRITE_COPY_ERROR = 2 };

typedef struct SaveMenuWriteTask {
    u8 unknown_00[16];
    struct SaveMenuWriteTask *parent;
    u8 unknown_14[12];
    int state, counter;
    u8 source_slot, destination_slot, checksum;
    s8 selection_mode;
    u8 previous_lock, previous_bit5, result, errors;
    u8 unknown_30[12];
    int child_result;
    u8 unknown_40[8];
} SaveMenuWriteTask;
typedef struct SaveMenuSlotSettings {
    u8 selected_slot : 4, occupied_slots : 4;
    u8 unknown_01[3];
} SaveMenuSlotSettings;
typedef struct SaveMenuBufferHeader {
    u8 signature[8];
    SaveMenuSlotSettings settings;
} SaveMenuBufferHeader;
typedef char SaveMenuWriteTask_SizeCheck[sizeof(SaveMenuWriteTask) == 72 ? 1 : -1];
typedef char SaveMenuSlotSettings_SizeCheck[sizeof(SaveMenuSlotSettings) == 4 ? 1 : -1];
typedef char SaveMenuBufferHeader_SizeCheck[sizeof(SaveMenuBufferHeader) == 12 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
/* selection_mode: negative clears occupancy, zero marks occupied, positive also selects the slot. */
int SaveMenuWrite_Start(SaveMenuWriteTask *parent, u8 source, u8 destination, u8 checksum, s8 selection_mode);
void SaveMenuWrite_Update(SaveMenuWriteTask *task);
void SaveMenuMessage_Show(SaveMenuText *text, u16 entry);
void SaveMenuMessage_Hide(SaveMenuText *text);
#ifdef __cplusplus
}
#endif
#endif
