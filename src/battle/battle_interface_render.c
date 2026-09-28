/*
 * Interface rendering (overlay 2, 0x02099598-0x020999D8).
 *
 * Drawing an interface sprite frame, a decimal number, and the intensity update
 * that dims or brightens the HUD.
 */

#include <nitro/mi_dma.h>
#include <game/battle_context.h>
#include <game/battle_effect.h>
#include <game/battle_object.h>

enum BattleInterfaceRenderConstant {
    BATTLE_INTERFACE_SPRITE_RESOURCE_ID = 3,
    BATTLE_INTERFACE_TEXTURE_FRAME_OFFSET = 0x68D0,
    BATTLE_INTERFACE_MAX_TEXTURE_FRAME = 7,
    BATTLE_INTERFACE_MAX_INTENSITY = 31
};

typedef struct BattleSpriteDisplayList {
    u8 unknown_00[8];
    u32 render_flags;
    BattleSpriteTransform transform;
    u32 render_parameters;
} BattleSpriteDisplayList;

extern BattleSpriteTransform gBattleSpriteFallbackTransform;
extern void func_0202cc58(const void *source, void *destination, u32 size);
extern void func_0202cd2c(const void *source, void *destination, u32 size);
extern void DC_FlushRange(const void *start, u32 size);

void *BattleSprite_DrawFrame(int frame, int intensity,
                             BattleSpriteTransform *transform,
                             int object_address, u16 z,
                             u16 resource_id, u16 render_flags) {
    BattleSceneObject *object = (BattleSceneObject *)object_address;
    BattleObjectDataLoadState *load_state;
    BattleSpriteDisplayList *display_list;
    BattlePosition position;
    int display_list_size;

    if (intensity < 1) {
        return 0;
    }
    if (intensity < BATTLE_INTERFACE_MAX_INTENSITY) {
        (*(u16 *)(gBattleContext +
                  BATTLE_INTERFACE_TEXTURE_FRAME_OFFSET))++;
    } else {
        intensity = BATTLE_INTERFACE_MAX_INTENSITY;
    }
    if (*(u16 *)(gBattleContext +
                 BATTLE_INTERFACE_TEXTURE_FRAME_OFFSET) >=
        BATTLE_INTERFACE_MAX_TEXTURE_FRAME + 1) {
        *(u16 *)(gBattleContext +
                 BATTLE_INTERFACE_TEXTURE_FRAME_OFFSET) = 1;
    }

    if (object != 0) {
        BattlePosition_StoreViewRelative(
            &position, object->x, (s16)(object->y - object->z),
            (s16)(object->effect_anchor_z +
                  16 * (256 - object->y)),
            object->flags.bits.use_raw_position,
            object->flags.bits.use_alternate_model);
        if (transform == 0) {
            transform = &gBattleSpriteFallbackTransform;
            transform->x = position.x << 8;
            transform->y = position.y << 8;
        } else {
            transform->x += position.x << 8;
            transform->y += position.y << 8;
        }

        transform->matrix[0] = 4096;
        transform->matrix[1] = 0;
        transform->matrix[2] = 0;
        transform->matrix[3] = 0;
        transform->matrix[4] = 0;
        transform->matrix[5] = 4096;
        transform->matrix[6] = 0;
        transform->matrix[7] = 0;
        transform->matrix[8] = 0;
        transform->matrix[9] = 0;
        transform->matrix[10] = 4096;
        transform->matrix[11] = 0;
        transform->z = z;
        *(u32 *)transform->unknown_3c = 4096;
    }

    load_state = BattleObjectData_GetLoadState(resource_id);
    display_list = (BattleSpriteDisplayList *)(
        ((u32 *)load_state->data)[frame]);
    if ((u8 *)transform < (u8 *)&display_list->transform) {
        func_0202cd2c(transform, &display_list->transform,
                      sizeof(*transform));
    } else {
        func_0202cc58(transform, &display_list->transform,
                      sizeof(*transform));
    }
    display_list->render_flags = render_flags;
    display_list->render_parameters =
        (display_list->render_parameters & 0xC0E0FFFF) |
        (*(u16 *)(gBattleContext +
                  BATTLE_INTERFACE_TEXTURE_FRAME_OFFSET) << 24) |
        (intensity << 16);
    display_list_size =
        ((u32 *)load_state->data)[frame + 1] -
        ((u32 *)load_state->data)[frame];
    DC_FlushRange(display_list, display_list_size);
    MI_SendGXCommand(1, display_list, display_list_size);
    return transform;
}

int BattleRender_UpdateIntensity(int direction, s16 *intensity) {
    int result = *intensity;

    if (direction == 0) {
        result -= 3;
    } else {
        result += 5;
    }
    if (result > BATTLE_INTERFACE_MAX_INTENSITY) {
        result = BATTLE_INTERFACE_MAX_INTENSITY;
    } else if (result < 0) {
        result = 0;
    }
    *intensity = result;
    return result;
}

int BattleNumber_DrawDecimal(int value, int intensity,
                             BattleSpriteTransform *transform,
                             int object_address, u16 render_flags,
                             int first_digit, int spacing) {
    BattleObjectDataLoadState *load_state;
    int digit_offset;
    int result = value;

    if (value < 0) {
        value = 0;
    }
    if (value >= 0 && intensity >= 1) {
        if (intensity < BATTLE_INTERFACE_MAX_INTENSITY) {
            (*(u16 *)(gBattleContext +
                      BATTLE_INTERFACE_TEXTURE_FRAME_OFFSET))++;
        } else {
            intensity = BATTLE_INTERFACE_MAX_INTENSITY;
        }
        if (*(u16 *)(gBattleContext +
                     BATTLE_INTERFACE_TEXTURE_FRAME_OFFSET) >=
            BATTLE_INTERFACE_MAX_TEXTURE_FRAME + 1) {
            *(u16 *)(gBattleContext +
                     BATTLE_INTERFACE_TEXTURE_FRAME_OFFSET) = 1;
        }
        load_state = BattleObjectData_GetLoadState(
            BATTLE_INTERFACE_SPRITE_RESOURCE_ID);
        digit_offset = 0;
        if (first_digit < 0) {
            first_digit = digit_offset;
        }
        {
            int frame_count =
                ((u32)((u32 *)load_state->data)[0] >> 2) -
                ((u32)load_state->data >> 2) - 2;
            int maximum_digit = frame_count - 9;
            if (maximum_digit < first_digit) {
                first_digit = maximum_digit;
            }
        }

        do {
            int digit = first_digit + value % 10;
            BattleSpriteDisplayList *display_list =
                (BattleSpriteDisplayList *)
                    ((u32 *)load_state->data)[digit];
            int display_list_size;
            int digit_width;

            if ((u8 *)transform < (u8 *)&display_list->transform) {
                func_0202cd2c(transform, &display_list->transform,
                              sizeof(*transform));
            } else {
                func_0202cc58(transform, &display_list->transform,
                              sizeof(*transform));
            }
            display_list->transform.x += digit_offset;
            display_list->render_flags = render_flags;
            display_list->render_parameters =
                (display_list->render_parameters & 0xC0E0FFFF) |
                (*(u16 *)(gBattleContext +
                          BATTLE_INTERFACE_TEXTURE_FRAME_OFFSET) << 24) |
                (intensity << 16);
            display_list_size =
                ((u32 *)load_state->data)[digit + 1] -
                ((u32 *)load_state->data)[digit];
            DC_FlushRange(display_list, display_list_size);
            MI_SendGXCommand(1, display_list, display_list_size);

            value /= 10;
            digit_width =
                transform->matrix[0] * ((spacing + 8) << 8);
            result = digit_width + ((u32)(digit_width >> 11) >> 20);
            digit_offset -= result >> 12;
        } while (value != 0);
    }

    return result;
}
