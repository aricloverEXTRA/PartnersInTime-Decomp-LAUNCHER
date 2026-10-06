/* Attached callback-renderer properties (overlay 2, 0x02068970..0x02068B50).
 * The caller supplies a live attachment and a valid transform index. Text
 * attachments own 148 bytes; cached sprites and number displays have smaller
 * payloads and expose only their transform. Unsupported properties return -1.
 */
#include <game/battle_callback_model.h>
#include <game/battle_text.h>
#include "battle_attached_properties_internal.h"

/* Kinds 2 and 5 share this allocation, and the getter intentionally reads the
 * clipped-text offsets for both, unlike their setters. */
struct TextPropertyFlags { u8 requested : 1, pending : 1, uploaded : 1, inhibited : 1, unknown : 4; };
/* A bounded view at payload + 12 * index preserves the native base-first
 * addressing; its transform starts four bytes into the view. */
struct TransformPropertyView {
    u32 prefix;
    s16 scale_x, scale_y;
    u16 rotation;
    s8 offset_x, offset_y;
    u32 mode : 2, unknown_mode : 30;
};
extern "C" int BattleSceneObject_GetAttachedModelProperty(BattleSceneObject *object, int property, int element)
{
    AttachedTextDisplay *display = (AttachedTextDisplay *)((BattleCallbackModelView *)object->alternate_model)->owned_buffer;
    if ((property & 0xf00) == 0x100) {
        #define TRANSFORM ((TransformPropertyView *)((u8 *)display + element * sizeof(BattleOamTransform)))
        switch (property) {
        case 0x100: return TRANSFORM->scale_x;
        case 0x101: return TRANSFORM->scale_y;
        case 0x102: return TRANSFORM->rotation;
        case 0x103: return TRANSFORM->offset_x;
        case 0x104: return TRANSFORM->offset_y;
        case 0x105: return TRANSFORM->mode;
        }
        #undef TRANSFORM
    }
    switch (display->kind) {
    case 2:
    case 5:
        switch (property) {
        case 2: return display->text.clipped.clip_left;
        case 3: return display->text.clipped.clip_right;
        case 4: return display->text.clipped.horizontal_mode;
        case 5: return display->text.clipped.vertical_mode;
        case 6: return display->property_84;
        case 7: return display->property_86;
        case 8: return display->property_88;
        case 9: return display->property_8e;
        case 10: return display->property_90;
        case 11: return display->property_92;
        case 12: return display->text.clipped.flags.bits.uploaded;
        case 13: return ((TextPropertyFlags *)&display->text.clipped.flags)->inhibited ^ 1;
        case 14: return display->mode;
        case 15: return !display->text.clipped.flags.bits.requested;
        case 16:
            switch (element) {
            case 0: return display->archive_id;
            case 1: return display->resource_id;
            }
            break;
        }
        break;
    case 3: break;
    }
    return -1;
}
