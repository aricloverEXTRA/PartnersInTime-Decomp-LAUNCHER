/*
 * Leading zero count (ARM9 resident, 0x0203B094-0x0203B09C).
 *
 * The single ARM9 word-at-a-time count the SDK exposes to C.
 */

#include <nitro.h>

u32 func_0203b094(u32 value)
{
    /* The SDK exposes CLZ as one instruction. */
    asm { clz value, value }
    return value;
}