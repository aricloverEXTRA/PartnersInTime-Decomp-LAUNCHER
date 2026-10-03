/*
 * Fast byte fill (ARM9 resident, unlinked draft).
 *
 * Unlinked draft. Body matches native size (148 B) but MWCC emits +4 bytes
 * due to control flow differences: native uses heavy conditional execution
 * (predicated instructions) and fall-through; MWCC emits explicit branches.
 * The alignment-peeling logic (odd byte, halfword, word loop, tail) is
 * correct, but MWCC cannot reproduce the predicated-instruction density.
 * Revisit when compiler rules for predicated code generation are understood.
 */

#include <nitro.h>

void MI_CpuFill8(void *destination, u8 value, u32 size)
{
    u32 *dst = (u32 *)destination;
    u32 pattern;
    u32 aligned;

    if (size == 0) {
        return;
    }

    if ((u32)destination & 1) {
        u16 half = *(u16 *)((u8 *)destination - 1);
        half = (half & 0xFF00) | value;
        *(u16 *)((u8 *)destination - 1) = half;
        destination = (u8 *)destination + 1;
        size--;
        if (size == 0) return;
    }

    pattern = (u32)value | ((u32)value << 8);

    if ((u32)destination & 2) {
        *(u16 *)destination = (u16)pattern;
        destination = (u8 *)destination + 2;
        size -= 2;
        if (size < 2) goto tail;
    }

    pattern = pattern | (pattern << 16);

    aligned = size & ~3u;
    if (aligned != 0) {
        u32 *end = dst + (aligned >> 2);
        do {
            *dst++ = pattern;
        } while (dst < end);
        destination = (u8 *)destination + aligned;
        size -= aligned;
    }

tail:
    if (size & 2) {
        *(u16 *)destination = (u16)pattern;
        destination = (u8 *)destination + 2;
    }
    if (size & 1) {
        *(u8 *)destination = value;
    }
}