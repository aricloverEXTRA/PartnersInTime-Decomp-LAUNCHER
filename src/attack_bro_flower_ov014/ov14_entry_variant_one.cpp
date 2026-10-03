/*
 * Bro Flower entry, variant one (overlay 14, 0x020C60E4-0x020C60F4).
 *
 * One of the two entry callbacks the attack loader hands to the overlay. Both
 * forward straight to the shared entry with a fixed variant selector, so the
 * variant only differs in which one the loader picks.
 */

#include "flower_internal.h"

extern "C" {
void func_ov014_020c60e4(Overlay10ActionActor *user)
{
    func_ov014_020c5e2c(user, 1);
}
}