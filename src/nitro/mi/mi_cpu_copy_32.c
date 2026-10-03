/*
 * Word-granular block copy (ARM9 resident, unlinked draft).
 *
 * The loop body already matches native; both "while" and "for" make MWCC
 * hoist the first comparison into "cmp size, #0 / bxeq lr", adding 8 bytes
 * that native does not have. A "do/while" moves the test below the body.
 * Native has neither - the hoist is suppressed for an unknown reason.
 * Revisit when that compiler rule is known.
 */

#include <nitro.h>

void func_0203b7b4(const void *source, void *destination, u32 size)
{
    const u32 *src = (const u32 *)source;
    u32 *dst = (u32 *)destination;
    u32 *end = dst + size;

    while (dst < end) {
        *dst++ = *src++;
    }
}