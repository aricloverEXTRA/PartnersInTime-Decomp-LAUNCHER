/*
 * Save panel resources (overlay 8, 0x02071964-0x02071AB0).
 *
 * Prepares location labels and queues copies of the location image buffers.
 */

#include "save_menu_internal.h"

extern u8 data_ov008_0207aa64[2];
extern u16 data_ov008_0207aa66[3];
void func_ov008_02071ab0(SaveMenuTransferTask *task);

SaveMenuTransferTask *SaveMenu_QueueLocationPanel(int panel, int image)
{
    SaveMenuTransferTask *task = func_ov005_0206659c(func_ov008_02071ab0, 11, 1);
    task->arguments[0] = panel;
    task->arguments[1] = image;
    return task;
}

SaveMenuTransferTask *SaveMenu_CopyLocationPanel(int panel, u32 source_panel)
{
    /* The first two entries track each panel's current image. Values at least
     * two select the shared third image buffer. */
    if (source_panel < 2)
        source_panel = data_ov008_0207aa64[source_panel];
    else
        source_panel = 2;
    data_ov008_0207aa64[panel] = source_panel;
    return SaveMenu_QueueLocationPanel(panel, source_panel);
}

typedef struct MenuLocationEntry {
    u16 value, unused;
} MenuLocationEntry;
typedef char MenuLocationEntrySizeCheck[sizeof(MenuLocationEntry) == 4 ? 1 : -1];
extern const MenuLocationEntry data_ov008_02078150[];
extern u16 data_ov008_0207aa6c[];

void SaveMenu_LoadLocationName(int slot, int location)
{
    int entry = data_ov008_02078150[location].value;
    int offset = (slot << 12) / 2 + 49152;
    SaveMenuText_DrawTextureRows(data_ov008_0207828c, &offset, 1, entry, 128, slot);
    data_ov008_0207aa6c[slot] = entry;
}

void SaveMenuText_PreparePanel(int panel, u16 entry)
{
    int offset;
    volatile u32 clear = 0;
    func_0203b7a0(clear, data_ov008_0207828c->pixels, 24576);
    offset = 10752 * panel;
    data_ov008_0207aa66[panel] = SaveMenuText_StreamObjectText(
        data_ov008_0207828c, DISPLAY_ENGINE_SUB, &offset, 2, entry);
}
