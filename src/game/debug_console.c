/*
 * Debug console (ARM9 resident, 0x0202ACC8-0x0202B1CC).
 *
 * A text console drawn into a background: cursor placement, printf-style
 * formatting and the tile upload. Left in the retail build but not reachable
 * from normal play.
 */

#include <game/format.h>
#include <game/debug_console.h>
#include <game/input.h>

extern GameDisplayWork data_0206032c;
extern const u8 data_02049fec[6144];
extern const u16 data_02049fcc[][2];
extern void MIi_CpuClearFast(u32 value, void *destination, u32 size);
extern void DC_FlushRange(const void *source, u32 size);
extern void func_02038170(const void *source, u32 offset, u32 size);
extern void func_020380a0(const void *source, u32 offset, u32 size);
extern void func_02037fd0(const void *source, u32 offset, u32 size);
extern void func_02037f00(const void *source, u32 offset, u32 size);
extern void func_02038108(const void *source, u32 offset, u32 size);
extern void func_02038038(const void *source, u32 offset, u32 size);
extern void func_02037f68(const void *source, u32 offset, u32 size);
extern void func_02037e98(const void *source, u32 offset, u32 size);
extern void func_02038640(const void *source, u32 offset, u32 size);
extern void func_020385d8(const void *source, u32 offset, u32 size);
extern void func_020384b0(const void *source, u32 offset, u32 size);
extern void func_020383e0(const void *source, u32 offset, u32 size);
extern void func_02038310(const void *source, u32 offset, u32 size);
extern void func_02038240(const void *source, u32 offset, u32 size);
extern void func_02038448(const void *source, u32 offset, u32 size);
extern void func_02038378(const void *source, u32 offset, u32 size);
extern void func_020382a8(const void *source, u32 offset, u32 size);
extern void func_020381d8(const void *source, u32 offset, u32 size);

static inline void SetBackground(volatile u16 *control, int character_base, int screen_base)
{
    *control = (*control & 0x43) | (screen_base << 8) | (character_base << 2);
    *control &= ~3;
}

GameConsole *GameConsole_Init(GameConsole *console, int sub_screen, int background,
                              int character_base, int screen_base, int palette, int color)
{
    console->sub_screen = sub_screen;
    console->background = background;
    console->palette = palette;
    if (!sub_screen) {
        switch (background) {
        case 0: SetBackground((vu16 *)0x04000008, character_base, screen_base); func_02038170(data_02049fec, 0, 6144); break;
        case 1: SetBackground((vu16 *)0x0400000a, character_base, screen_base); func_020380a0(data_02049fec, 0, 6144); break;
        case 2: SetBackground((vu16 *)0x0400000c, character_base, screen_base); func_02037fd0(data_02049fec, 0, 6144); break;
        case 3: SetBackground((vu16 *)0x0400000e, character_base, screen_base); func_02037f00(data_02049fec, 0, 6144); break;
        }
        func_02038640(data_02049fcc[color], palette * 32, 4);
    } else if (data_0206032c.display_mode != 1) {
        switch (background) {
        case 0: SetBackground((vu16 *)0x04001008, character_base, screen_base); func_02038108(data_02049fec, 0, 6144); break;
        case 1: SetBackground((vu16 *)0x0400100a, character_base, screen_base); func_02038038(data_02049fec, 0, 6144); break;
        case 2: SetBackground((vu16 *)0x0400100c, character_base, screen_base); func_02037f68(data_02049fec, 0, 6144); break;
        case 3: SetBackground((vu16 *)0x0400100e, character_base, screen_base); func_02037e98(data_02049fec, 0, 6144); break;
        }
        func_020385d8(data_02049fcc[color], palette * 32, 4);
    }
    return console;
}

void GameConsole_Clear(GameConsole *console)
{
    volatile u32 zero = 0;
    MIi_CpuClearFast(zero, console, 1536);
}

void GameConsole_Upload(GameConsole *console)
{
    DC_FlushRange(console, 1536);
    if (!console->sub_screen) {
        switch (console->background) {
        case 0: func_020384b0(console, 0, 1536); break;
        case 1: func_020383e0(console, 0, 1536); break;
        case 2: func_02038310(console, 0, 1536); break;
        case 3: func_02038240(console, 0, 1536); break;
        }
    } else if (data_0206032c.display_mode != 1) {
        switch (console->background) {
        case 0: func_02038448(console, 0, 1536); break;
        case 1: func_02038378(console, 0, 1536); break;
        case 2: func_020382a8(console, 0, 1536); break;
        case 3: func_020381d8(console, 0, 1536); break;
        }
    }
}

void GameConsole_SetCursor(GameConsole *console, int x, int y)
{
    console->x = x;
    console->y = y;
}

void GameConsole_Printf(GameConsole *console, const char *format, ...)
{
    /* The ARM compiler saves the argument registers when their address is taken. */
    const u32 *arguments = (const u32 *)(((u32)&format & ~3) + 4);
    GameFormat_WriteArguments(console->text, format, arguments);
    GameConsole_Write(console, console->text);
}

void GameConsole_Write(GameConsole *console, const char *text)
{
    int x;
    int y = console->y;
    int row;
    u16 *tile;
    x = console->x;
    row = y * 32;
    tile = &console->tiles[0][x + row];
    while (*text) {
        u16 glyph = (u8)*text++;
        if (glyph == '\n') {
            x = console->x;
            row += 32;
            ++y;
            tile = &console->tiles[0][x + row];
        } else {
            glyph -= 32;
            ++x;
            *tile++ = glyph | (console->palette << 12);
        }
    }
    console->y = y;
    console->x = x;
}
