/*
 * String formatting (ARM9 resident, 0x0202A914-0x0202AA74).
 *
 * The hex and string conversions the game's formatting helpers are built from.
 */

#include <game/format.h>

extern const char *data_02059b60[2];
extern const char data_02059bb8[];

u8 *GameFormat_Hex(u8 *destination, int width, int flags, u32 value)
{
    const char *alphabet = data_02059b60[flags >> 1];
    u8 padding = (flags & 1) ? '0' : ' ';
    int count = 0;
    u8 digits[8];
    u8 *cursor;
    if (width > 8) width = 8;
    cursor = digits;
    do {
        ++count;
        *cursor++ = alphabet[value & 15];
        value >>= 4;
    } while ((!width || count < width) && value);
    if (width > 0 && count < width) {
        cursor = digits + count;
        do {
            ++count;
            *cursor++ = padding;
        } while (count < width);
    }
    cursor = digits + count;
    do {
        *destination++ = *--cursor;
        --count;
    } while (count > 0);
    *destination = 0;
    return destination;
}

char *GameFormat_String(char *destination, int width, int flags, const char *text)
{
    int length;
    if (!text) text = data_02059bb8;
    length = GameFormat_Length(text);
    for (; width > length; ++destination) {
        --width;
        *destination = ' ';
    }
    do {
        *destination++ = *text;
        if (!*++text) break;
    } while (width <= 0 || --width);
    *destination = 0;
    return destination;
}

/* Match the ARM varargs layout used by GameConsole_Printf. Taking the address
 * of the last named argument makes MWCC spill r0-r3; the first optional word
 * then precedes the caller's stack arguments in the same contiguous area. */
char *GameFormat_Write(char *destination, const char *format, ...)
{
    const u32 *arguments = (const u32 *)(((u32)&format & ~3) + 4);
    return GameFormat_WriteArguments(destination, format, arguments);
}
