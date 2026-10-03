/*
 * Bro Flower entry, variant zero (overlay 14, 0x020C60F4-0x020C6104).
 *
 * The companion to the variant-one callback; it forwards to the same shared
 * entry with the other selector.
 */

#include "flower_internal.h"

extern "C" {
void func_ov014_020c60f4(Overlay10ActionActor *user)
{
    func_ov014_020c5e2c(user, 0);
}
}