/*
 * Attached text upload start (ov002, 0x0206AC34-0x0206ACBC).
 * Consumes a queued clipped-text resource request once the interface layer
 * reports the resource ready: the request becomes a pending upload, the scroll
 * phase and its counter restart from zero, the Q8 position is reseeded from
 * property_8e and the stored pixel extent resolves from the queued value, with
 * -2 keeping it and -1 taking the cursor's measured width.
 */

#include <nitro.h>
#include "battle_attached_properties_internal.h"

void func_ov002_0206ac34(AttachedTextDisplay *display)
{
    if (display->text.clipped.flags.bits.requested != 1)
        return;
    if (!display->text.clipped.text_flags.bits.resource_ready)
        return;
    display->text.clipped.flags.bits.requested = 0;
    display->text.clipped.flags.bits.pending = 1;
    display->unknown_8a = 0;
    display->unknown_8c = 0;
    display->position_q8 = display->property_8e << 8;
    if (display->text.clipped.unknown_3c == -2)
        return;
    switch (display->text.clipped.unknown_3c) {
    case -1:
        display->text.clipped.unknown_3a = display->text.clipped.text.cursor.bits.x;
        return;
    default:
        display->text.clipped.unknown_3a = display->text.clipped.unknown_3c;
        return;
    }
}
