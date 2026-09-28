/* Fill the 1024-bit extended field-script save-flag bank. */
extern "C" {
#include <game/save_data.h>
#include <game/field_script.h>
extern void func_0202cbd4(void *destination, u32 value, u32 size);
void FieldVm_FillExtendedSaveFlags(int enabled)
{
    u32 value;
    if (enabled) value = 0xffffffffu;
    else value = 0;
    func_0202cbd4(gSaveData + 0x270, value, 0x80);
}
}
