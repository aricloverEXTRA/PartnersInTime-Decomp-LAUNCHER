#ifndef PIT_GAME_FORMAT_H
#define PIT_GAME_FORMAT_H

/*
 * The game's own number and string formatting helpers.
 */

#include <nitro.h>

#ifdef __cplusplus
extern "C" {
#endif

int GameFormat_Length(const char *text);
u8 *GameFormat_Binary(u8 *destination, int width, int flags, u32 value);
u8 *GameFormat_Decimal(u8 *destination, int width, int flags, u32 value);
u8 *GameFormat_Hex(u8 *destination, int width, int flags, u32 value);
char *GameFormat_String(char *destination, int width, int flags, const char *text);

/* Return the cursor immediately after the terminating NUL, not a character
 * count. These native helpers do not accept a destination-capacity argument. */
char *GameFormat_Write(char *destination, const char *format, ...);
char *GameFormat_WriteArguments(char *destination, const char *format,
                                const u32 *arguments);

#ifdef __cplusplus
}
#endif
#endif
