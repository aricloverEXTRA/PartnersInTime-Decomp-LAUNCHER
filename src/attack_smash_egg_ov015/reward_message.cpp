/* Smash Eggs reward-message construction, overlay 15, 0x020C2BB8-0x020C2E78. */

#include "actor_internal.h"
extern "C" {
#include <game/battle_window.h>
#include <game/battle_effect.h>
#include <game/battle_attack_loader.h>
void func_0202cbd4(void *, int, u32);
void func_02015ddc(const void *, void *, u32);
void func_02015ef4(const void *, void *, u32);
}

struct RewardWindowWork {
    u8 unknown_000[0x208];
    BattleWindowInterface *windows;
};

/* Keep the engine's directional copies. The backward helper also writes a
 * one-to-four-byte carry past the name; the 128-byte message has room for it. */
static inline void CopyRewardName(const void *source, void *destination)
{
    if ((u32)source < (u32)destination)
        func_02015ef4(source, destination, 32);
    else
        func_02015ddc(source, destination, 32);
}

extern "C" int Overlay15Attack_OpenRewardMessage(const Overlay15AttackRewardItemPrefix *item)
{
    GameWindowProperties request;
    GameText text;
    GameTextBounds bounds;
    int category;
    Overlay15AttackContext *context = data_ov002_020c0710;
    const void *name;
    u8 *message;
    int width;
    func_0202cbd4(&request, 0, sizeof(request));
    /* Reward records use one of the four supported inventory categories. */
    switch (item->item_id & 0xf000) {
    case 0x1000: category = 2; break;
    case 0x2000: category = 6; break;
    case 0x3000: category = 9; break;
    case 0x4000: category = 11; break;
    }
    name = BattleText_GetEntry((u16)category, item->name_id);
    /* Control prefix reserves the icon gap before the fixed-size item name. */
    message = context->message;
    message[0] = 0xff;
    message[1] = 0x26;
    message[2] = 0xff;
    message[3] = 0x35;
    message[4] = 0xff;
    message[5] = 0x0b;
    message[6] = 1;
    message[7] = 0xff;
    message[8] = 0x58;
    message[9] = 0x20;
    message[10] = 0x20;
    message[11] = 0xff;
    message[12] = 0x56;
    message[13] = 0xff;
    message[14] = 3;
    CopyRewardName(name, message + 15);
    func_0202cbd4(&text, 0, sizeof(text));
    func_0202cbd4(&bounds, 0, sizeof(bounds));
    GameText_Init(&text, (const u32 *const *)(gBattleContext + 0x68f4), 0,
        context->message, 0, 0, 1, 1, 6, 6, 63, 0, 255, 0);
    GameText_MeasureBounds(&text, &bounds);
    width = bounds.width;
    context->message_width = width;
    request.string = context->message;
    request.shape.bits.screen = 0;
    request.shape.bits.skin = 3;
    request.shape.bits.width = 0;
    request.shape.bits.height = 0;
    /* Centre on the native 114-pixel anchor after rounding to whole tiles. */
    request.position.bits.x = 114 - ((width + 7) & ~7) / 2;
    request.position.bits.y = 8;
    request.position.bits.reserved18 = 1;
    request.layout.bits.mode = 0;
    request.layout.bits.flag4 = 0;
    request.layout.bits.style = 0;
    request.layout.bits.flag9 = 0;
    request.layout.bits.width = 0;
    request.layout.bits.extent = 1023;
    request.value = 0;
    request.reserved18 = 0;
    request.shape.bits.sound = 0;
    request.fonts = (const u32 *const *)(gBattleContext + 0x68f4);
    return ((RewardWindowWork *)data_ov002_020c0660)->windows->open(&request, -1);
}
