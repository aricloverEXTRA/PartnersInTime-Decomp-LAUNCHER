/* Apply the shop's horizontal wave only to its two scrolling background bands. */
#include "shop_scene_internal.h"

extern "C" void ShopScene_UpdateHBlank(void)
{
    int line = REG16(0x04000006);
    int offset = data_ov009_0207f23c.sub;
    if (line < offset + 31) REG32(0x04001028) = 0;
    else if (line < offset + 96) REG32(0x04001028) = data_ov009_0207ea3c.wave_current;
    else if (line < offset + 119) REG32(0x04001028) = 0;
    else if (line < offset + 192) REG32(0x04001028) = data_ov009_0207ea3c.wave_current;
    else REG32(0x04001028) = 0;
}
