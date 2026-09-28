#ifndef PIT_GAME_MENU_EQUIPMENT_H
#define PIT_GAME_MENU_EQUIPMENT_H
#include <nitro.h>
enum MenuEquipmentPanelState {
    MENU_EQUIPMENT_ENTER = 0,
    MENU_EQUIPMENT_IDLE = 100,
    MENU_EQUIPMENT_SWITCH_OUT = 101,
    MENU_EQUIPMENT_SWITCH_IN = 102,
    MENU_EQUIPMENT_CLOSE = 1000,
    MENU_EQUIPMENT_EXIT = 1001
};
enum MenuEquipmentComparison {
    MENU_EQUIPMENT_UNAVAILABLE = -1,
    MENU_EQUIPMENT_SAME,
    MENU_EQUIPMENT_HIGHER,
    MENU_EQUIPMENT_LOWER
};
/* Panel positions and velocities use twelve fractional bits. */
typedef struct MenuEquipmentTask {
    u8 unknown_00[16];
    struct MenuEquipmentTask *parent;
    u8 unknown_14[12];
    int state, counter;
    union {
        struct {
            s8 member, closing;
            u16 unknown_2a;
            int x, y, velocity, acceleration, target;
            u8 unknown_40[8];
        } panel;
        struct {
            s8 member, closing;
            u16 unknown_2a;
            int x, y, target_y, step_y;
            u8 unknown_3c[12];
        } heading;
        struct {
            u8 animation, stat;
            s16 x, y;
            u16 unknown_2e;
            int unknown_30, unknown_34;
            u32 current, preview;
            u8 unknown_40[8];
        } row;
        struct {
            int x, y, amplitude, angle, direction;
            u8 unknown_3c[12];
        } member_arrow;
    };
} MenuEquipmentTask;

typedef char MenuEquipmentTask_SizeCheck[sizeof(MenuEquipmentTask) == 72 ? 1 : -1];
#ifdef __cplusplus
extern "C" {
#endif
void MenuEquipment_RequestClose(void);
void MenuEquipment_CreateHeading(void);
void MenuEquipment_UpdateComparisonArrow(MenuEquipmentTask *task);
void MenuEquipment_UpdateNumberStrip(MenuEquipmentTask *task);
/* stat must be 0..4: HP, power, defense, speed or stache. */
void MenuEquipment_UpdateStatRow(MenuEquipmentTask *task);
void MenuEquipment_UpdateStatsPanel(MenuEquipmentTask *task);
void MenuEquipment_UpdateHeading(MenuEquipmentTask *task);
void MenuEquipment_UpdateMemberArrow(MenuEquipmentTask *task);
#ifdef __cplusplus
}
#endif
#endif
