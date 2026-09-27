/*
 * Area script startup and update gate (overlay 0, 0x0207EB28-0x0207EBC4).
 *
 * Starts scripts when the room is ready and updates them while control is free.
 */

#include <game/field_area.h>
extern void func_ov000_02088c88(FieldScriptManager *);
typedef struct FieldOwnerModeView {
    u8 prefix[600];
    u16 mode : 4, reserved : 12;
} FieldOwnerModeView;

void FieldArea_StartScripts(FieldAreaContext *area)
{
    FieldScriptManager_Init(&area->scripts, area, area->startup_script);
    FieldScriptManager_StartEntityScripts(&area->scripts);
}

void FieldArea_UpdateScriptsWhenIdle(FieldAreaContext *area) {
    if ((!area->auxiliary || !((FieldOwnerModeView *)area->auxiliary)->mode) &&
        area->unknown_23f0 == 71)
        func_ov000_02088c88(&area->scripts);
}
