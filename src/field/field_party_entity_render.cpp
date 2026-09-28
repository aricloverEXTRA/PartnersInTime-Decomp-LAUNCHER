/*
 * Party entity rendering (overlay 0, 0x020B7EF8-0x020B85F8).
 *
 * Animation selection, screen positions, auxiliary priorities and sort keys on the
 * anchors, the blink renderers, and the state resources bound at offsets 18 and
 * 1C.
 */

#include <game/field_resources.h>
#include <game/field_party.h>
#include <game/field_auxiliary.h>
#include <game/field_blink.h>
#include <game/field_presentation.h>

/* State 21 tests only the first word of these two records. The contact
 * record's remaining layout is unknown. */
struct FieldPartyActionFlagPrefix { u32 enabled : 1, unknown : 31; };
struct FieldPartyContactFlagPrefix { u32 unknown_0_2 : 3, unknown_3_6 : 4, unknown_7_31 : 25; };
typedef char FieldPartyActionFlagPrefix_SizeCheck[sizeof(FieldPartyActionFlagPrefix) == 4 ? 1 : -1];
typedef char FieldPartyContactFlagPrefix_SizeCheck[sizeof(FieldPartyContactFlagPrefix) == 4 ? 1 : -1];

extern "C" {
void func_ov000_020a4a1c(FieldRuntimeEntity *, int, u8);
void func_0200940c(FieldRenderObject *, int);
void FieldEntity3D_UpdateScreenPosition(FieldRuntimeEntity *, s16, s16);
void GameAudio_PlayEffectDelayed(int, int, int);

/* Special locomotion states select a resource group or delegate to the base
 * animation policy. Preserve the renderer's behavior across a resource change,
 * then synchronize enabled auxiliary entities with the original refresh flag.
 * Separate equivalent switch arms retain the original dispatcher layout. */
void FieldPartyEntity_UpdateAnimation(FieldPartyEntity *member, int mode, u8 update_bounds)
{
    unsigned int animation;
    unsigned int refresh = member->entity.saved_presentation_flag_bits.unknown_14;
    if (member->entity.locomotion_state <= 3) {
        func_ov000_020a4a1c(&member->entity, mode, update_bounds);
    } else {
        int suppressed = mode == 6;
        int handled = 0;
        if (!member->entity.primary_resource_record->flag_bits.direction_mode)
            animation = member->entity.animation_id;
        else
            animation = member->entity.base_state_flag_bits.facing_direction;
        switch (member->entity.locomotion_state) {
        case 4:
        case 5:
        case 6:
        case 8:
            func_ov000_020a4a1c(&member->entity,
                member->entity.default_vertical_launch_velocity >= 0 ? 4 : 5, update_bounds);
            handled = 1;
            break;
        case 7:
        case 9:
            func_ov000_020a4a1c(&member->entity, 3, update_bounds);
            handled = 1;
            break;
        case 10:
        case 11:
        case 12:
        case 13:
        case 15:
        case 16:
        case 17:
        case 18:
            animation += 8;
            break;
        case 21:
            if (member->state_record &&
                ((FieldPartyActionFlagPrefix *)member->state_record)->enabled) {
                const FieldPartyContactFlagPrefix *contact =
                    (const FieldPartyContactFlagPrefix *)member->entity.unknown_508;
                if (!contact || contact->unknown_3_6)
                    animation += 8;
            }
            break;
        case 25:
            animation += 8;
            break;
        case 26:
            animation += 16;
            break;
        case 28:
        case 29:
        case 30:
        case 32:
            func_ov000_020a4a1c(&member->entity,
                member->entity.default_vertical_launch_velocity >= 0 ? 4 : 5, update_bounds);
            handled = 1;
            break;
        case 31:
        case 33:
            func_ov000_020a4a1c(&member->entity, 3, update_bounds);
            handled = 1;
            break;
        case 38:
            func_ov000_020a4a1c(&member->entity, 4, update_bounds);
            handled = 1;
            break;
        case 39:
            func_ov000_020a4a1c(&member->entity,
                member->entity.default_vertical_launch_velocity >= 0 ? 4 : 5, update_bounds);
            handled = 1;
            break;
        case 42:
        case 44:
            func_ov000_020a4a1c(&member->entity, 3, update_bounds);
            handled = 1;
            break;
        case 47:
            animation += 8;
            break;
        case 53:
        case 54:
        case 56:
            func_ov000_020a4a1c(&member->entity, 3, update_bounds);
            handled = 1;
            break;
        case 57:
        case 58:
            func_ov000_020a4a1c(&member->entity,
                member->entity.default_vertical_launch_velocity >= 0 ? 4 : 5, update_bounds);
            handled = 1;
            break;
        case 65:
        case 72:
            func_ov000_020a4a1c(&member->entity, 3, update_bounds);
            handled = 1;
            break;
        case 67:
            func_ov000_020a4a1c(&member->entity, 3, update_bounds);
            handled = 1;
            break;
        case 77:
            animation += 8;
            break;
        case 80:
            func_ov000_020a4a1c(&member->entity, 0, update_bounds);
            handled = 1;
            break;
        case 83:
            func_ov000_020a4a1c(&member->entity, 3, update_bounds);
            handled = 1;
            break;
        case 86:
            animation += 8;
            break;
        case 88:
        case 90:
            func_ov000_020a4a1c(&member->entity, 3, update_bounds);
            handled = 1;
            break;
        case 91:
            func_ov000_020a4a1c(&member->entity, 4, update_bounds);
            handled = 1;
            break;
        case 92:
            func_ov000_020a4a1c(&member->entity, 3, update_bounds);
            handled = 1;
            break;
        case 93:
            func_ov000_020a4a1c(&member->entity, 5, update_bounds);
            handled = 1;
            break;
        }
        if (!handled) {
            if (member->entity.saved_presentation_flag_bits.unknown_14 ||
                member->entity.animation_id != animation) {
                unsigned int behavior = member->entity.render_object->state_flag_bits.behavior_state;
                FieldEntity_SetResourceAnimation(&member->entity, animation, -1, 1, update_bounds);
                member->entity.render_object->state_flag_bits.behavior_state = (u8)behavior;
                member->entity.saved_presentation_flag_bits.unknown_14 = 0;
            }
            member->entity.render_object->state_flag_bits.animation_suppressed = (u8)suppressed;
        }
    }
    for (int i = 0; i < 6; ++i) {
        FieldAuxiliaryEntity *aux = member->auxiliaries[i];
        if (aux && aux->entity.base_state_flag_bits.animation_wait_enabled) {
            int auxiliary_mode;
            if (aux->bits.follow_locomotion) {
                aux->entity.saved_presentation_flag_bits.unknown_14 = refresh;
                func_0200940c(member->auxiliaries[i]->entity.render_object,
                    member->entity.render_object->animation_speed);
                auxiliary_mode = mode;
            } else {
                auxiliary_mode = 0;
            }
            member->auxiliaries[i]->entity.base.update_animation(auxiliary_mode, update_bounds);
        }
    }
}

void FieldPartyEntity_UpdateScreenPositions(FieldPartyEntity *member, s16 camera_x, s16 camera_y)
{
    FieldEntity3D_UpdateScreenPosition(&member->entity, camera_x, camera_y);
    for (int i = 0; i < 6; i++)
        if (member->auxiliaries[i])
            member->auxiliaries[i]->entity.base.update_screen_position(camera_x, camera_y);
}

void FieldPartyEntity_UpdateAuxiliaryPriorities(FieldPartyEntity *member)
{
    for (int i = 0; i < 6; i++)
        if (member->auxiliaries[i])
            FieldAuxiliary_UpdateRenderPriority(member->auxiliaries[i]);
}

void FieldPartyEntity_CopySortKeysToAnchors(FieldPartyEntity *member)
{
    FieldEntity_CopySortKeysToAnchors(&member->entity);
    for (int i = 0; i < 6; i++)
        if (member->auxiliaries[i])
            member->auxiliaries[i]->entity.base.unknown_70();
}

void FieldPartyEntity_ShowBlinkRenderersWithSound(FieldEntity *entity)
{
    FieldBlink_ShowRenderers(entity);
    GameAudio_PlayEffectDelayed(226, 0, -1);
}

void FieldPartyEntity_HideBlinkRenderers(FieldEntity *entity)
{
    FieldBlink_HideRenderers(entity);
}

void FieldPartyEntity_BindStateResource18(FieldPartyEntity *member)
{
    FieldEntity_RebindRendererResources(&member->entity,
        (const FieldPrimaryResource *)member->state_record->resources.resource_18, 0, 0, -1, 0, 0);
}

void FieldPartyEntity_BindStateResource1C(FieldPartyEntity *member)
{
    FieldEntity_RebindRendererResources(&member->entity,
        (const FieldPrimaryResource *)member->state_record->resources.resource_1c, 0, 0, -1,
                        member->state_record->resources.flags.update_bounds != 0, 0);
    member->state_record->resources.flags.update_bounds = 0;
}
}
