/*
 * Save context initialization (ARM9 resident, 0x02005BA8-0x02005D74).
 *
 * Allocates the live context once, applies the supplied or default session
 * settings, and initializes the party and localized text resources.
 */

extern "C" {
#include <nitro.h>
#include <game/heap.h>
#include <game/session.h>
#include <game/save_data.h>
#include <game/inventory.h>
#include <game/text.h>

typedef struct SessionSaveContext {
    u8 unknown_000[0x48];
    u32 script_values[2];
    u8 unknown_050[0x4c4];
    u8 option_2 : 1, option_9 : 1, reserved_2_5 : 4, rumble : 1, reserved_7 : 1;
    u8 language;
    u8 unknown_516[0x66];
    GameSessionSettings settings;
} SessionSaveContext;
typedef char SizeCheck[sizeof(SessionSaveContext) == 0x58c ? 1 : -1];
extern const GameSessionSettings data_02048f08;
extern u8 data_0205a00c;
void func_0202cbd4(void *destination, int value, u32 size);
void MI_CpuCopy8(const void *source, void *destination, u32 size);

#define SAVE ((SessionSaveContext *)gSaveData)

void GameSession_InitializeSaveContext(const GameSessionSettings *settings)
{
    if (gSaveData) return;
    gSaveData = (u8 *)GameHeap_New(sizeof(SessionSaveContext), 1, 0, 0);
    func_0202cbd4(gSaveData, 0, sizeof(SessionSaveContext));
    /* C++ memberwise assignment preserves the padding halfword at +6. */
    if (settings) {
        SAVE->settings = *settings;
        MI_CpuCopy8(settings->unknown_08, SAVE->script_values, 8);
    } else {
        SAVE->settings = data_02048f08;
        SAVE->settings.option_3 = data_0205a00c;
    }
    SAVE->option_2 = (u8)SAVE->settings.option_2;
    SAVE->language = SAVE->settings.language;
    SAVE->option_9 = (u8)SAVE->settings.unknown_9;
    SAVE->rumble = (u8)SAVE->settings.option_3;
    GameParty_Initialize(0);
    GameTextResources_LoadAll();
}
}
