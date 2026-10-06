#ifndef PIT_BATTLE_ATTACHED_PROPERTIES_INTERNAL_H
#define PIT_BATTLE_ATTACHED_PROPERTIES_INTERNAL_H

/* The payload behind a callback-renderer attachment: the two OAM transforms it
 * draws with, the clipped or tiled text they carry and the scroll state that
 * drives it. Text attachments own 148 bytes; cached sprites and number
 * displays have smaller payloads and expose only their transform. */
#include <game/battle_text.h>
#include "battle_oam_internal.h"

typedef struct AttachedTextDisplay {
    u32 kind : 4, unknown_kind : 28;
    BattleOamTransform transforms[2];
    union { BattleClippedText clipped; BattleTiledText tiled; } text;
    u16 unknown_78;
    u8 archive_id, unknown_7b;
    u16 resource_id, unknown_7e;
    s32 position_q8;
    s16 property_84;
    u16 property_86, property_88;
    u8 unknown_8a;
    u8 mode : 2, unknown_mode : 6;
    u16 unknown_8c;
    s16 property_8e, property_90, property_92;
} AttachedTextDisplay;

typedef char DisplaySize[sizeof(AttachedTextDisplay) == 148 ? 1 : -1];

#endif
