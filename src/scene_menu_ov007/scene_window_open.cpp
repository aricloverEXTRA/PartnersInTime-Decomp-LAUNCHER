/*
 * Scene window open (overlay 7, 0x02089670-0x02089980).
 *
 * Opens the scene's message window and reports whether one is open.
 */

#include "scene_window_internal.h"

extern "C" {

int SceneWindow_Open(int text, int screen, int x, int y, int skin, int mode, int flag4, int style, int flag9,
                     int line_width, int width, int height, int flag18, int sound, int index, int value,
                     u32 unknown18)
{
    GameWindowProperties properties;
    const u8 *entry;
    const u8 *table;
    GameWindow_ResetProperties(MANAGER, &properties);
    table = *(const u8 **)(data_ov007_020a6b90 + 8);
    entry = table + ((const u32 *)table)[text];
    if (!width)
        width = entry[0] + 1;
    if (!height)
        height = entry[1] + 1;
    if (flag4 == -1)
        flag4 = height > 2;
    if (style == -1)
        style = y + 8 * height >= 112;
    if (line_width == -1)
        line_width = 0;
    if ((x & 0xffff) == 0x8000)
        x = (256 - 8 * width) / 2;
    if (width > 0)
        --width;
    if (height > 0)
        --height;
    properties.value = value;
    properties.shape.bits.screen = screen;
    properties.shape.bits.skin = skin;
    properties.shape.bits.width = width;
    properties.shape.bits.height = height;
    properties.shape.bits.position_mode = 0;
    properties.shape.bits.sound = sound;
    properties.layout.bits.mode = mode;
    properties.layout.bits.flag4 = flag4;
    properties.layout.bits.style = style;
    properties.layout.bits.flag9 = flag9;
    properties.layout.bits.width = line_width;
    properties.layout.bits.extent = text;
    properties.position.bits.x = x;
    properties.position.bits.y = y;
    properties.position.bits.reserved18 = flag18;
    properties.fonts = (const u32 *const *)(data_ov007_020a6b90 + 12);
    properties.string = *(const u8 **)(data_ov007_020a6b90 + 8);
    properties.reserved18 = unknown18;
    return INTERFACE->open(&properties, (s16)index);
}

int SceneWindow_IsOpen(int index)
{
    if (index >= 0)
        return ((u8)MANAGER->windows[(s16)index].allocation.bits.closing ^ 1) != 0;
    for (int i = 0; i < 4; ++i)
        if (!(u8)MANAGER->windows[(s16)i].allocation.bits.closing)
            return 1;
    return 0;
}
}
