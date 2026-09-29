/*
 * Auxiliary entity updates (overlay 0, 0x020B6560-0x020B6E98).
 *
 * The per-frame work for an auxiliary entity, all of it evaluated at the offset
 * it is attached at: navigation, collision, contacts, renderer position,
 * animation and render priority.
 */

#include <game/field_auxiliary.h>

extern "C" {

#include <game/heap.h>
extern FieldEntityVTable data_ov000_020c1194;
extern const u8 data_ov000_020c0c00[];
void func_0202cbd4(void *, int, u32);
void func_ov000_020a9d1c(FieldRuntimeEntity *, u8);
void func_ov000_020ae0c0(FieldRuntimeEntity *);
void func_ov000_020ae6c0(FieldRuntimeEntity *, int);
int func_ov000_020b4728(FieldRuntimeEntity *, int, int);

extern const u8 data_ov000_020c0b78[];
void func_ov000_020a4a1c(FieldRuntimeEntity *, int, u8);

/* Local offsets are applied only around the existing spatial helpers. */
int FieldAuxiliary_TestNavigationAtOffset(FieldAuxiliaryEntity *aux, int mask, int mode)
{
    int result;
    aux->entity.position_x += aux->offset_x;
    aux->entity.position_y += aux->offset_y;
    aux->entity.position_z += aux->offset_z;
    aux->entity.previous_position_x += aux->previous_offset_x;
    aux->entity.previous_position_y += aux->previous_offset_y;
    aux->entity.previous_position_z += aux->previous_offset_z;
    result = func_ov000_020b4728(&aux->entity, mask, mode);
    aux->entity.position_x -= aux->offset_x;
    aux->entity.position_y -= aux->offset_y;
    aux->entity.position_z -= aux->offset_z;
    aux->entity.previous_position_x -= aux->previous_offset_x;
    aux->entity.previous_position_y -= aux->previous_offset_y;
    aux->entity.previous_position_z -= aux->previous_offset_z;
    return result;
}

void FieldAuxiliary_UpdateCollisionAtOffset(FieldAuxiliaryEntity *aux, int mode)
{
    if (aux->entity.base.property_00a_bits.property_00a_flag_00 &&
        (aux->entity.collision_state_flag_bits.unknown_03 ||
         aux->entity.collision_state_flag_bits.unknown_07)) {
        aux->entity.position_x += aux->offset_x;
        aux->entity.position_y += aux->offset_y;
        aux->entity.position_z += aux->offset_z;
        aux->entity.previous_position_x += aux->previous_offset_x;
        aux->entity.previous_position_y += aux->previous_offset_y;
        aux->entity.previous_position_z += aux->previous_offset_z;
        func_ov000_020ae6c0(&aux->entity, mode);
        aux->entity.position_x -= aux->offset_x;
        aux->entity.position_y -= aux->offset_y;
        aux->entity.position_z -= aux->offset_z;
        aux->entity.previous_position_x -= aux->previous_offset_x;
        aux->entity.previous_position_y -= aux->previous_offset_y;
        aux->entity.previous_position_z -= aux->previous_offset_z;
    }
}

void FieldAuxiliary_UpdateContactsAtOffset(FieldAuxiliaryEntity *aux)
{
    if (aux->entity.base.property_00a_bits.property_00a_flag_00 &&
        (aux->entity.runtime_flag_bits.horizontal_sync_dirty ||
         aux->entity.runtime_flag_bits.vertical_sync_dirty || aux->entity.runtime_flag_bits.sync_horizontal ||
         aux->entity.runtime_flag_bits.sync_vertical)) {
        aux->entity.position_x += aux->offset_x;
        aux->entity.position_y += aux->offset_y;
        aux->entity.position_z += aux->offset_z;
        aux->entity.previous_position_x += aux->previous_offset_x;
        aux->entity.previous_position_y += aux->previous_offset_y;
        aux->entity.previous_position_z += aux->previous_offset_z;
        func_ov000_020ae0c0(&aux->entity);
        aux->entity.position_x -= aux->offset_x;
        aux->entity.position_y -= aux->offset_y;
        aux->entity.position_z -= aux->offset_z;
        aux->entity.previous_position_x -= aux->previous_offset_x;
        aux->entity.previous_position_y -= aux->previous_offset_y;
        aux->entity.previous_position_z -= aux->previous_offset_z;
        if (aux->entity.locomotion_state == 94 && !aux->entity.runtime_flag_bits.contact_mask_b &&
            !aux->entity.unknown_3a0_bits.contact_mask_a &&
            aux->entity.position_x == aux->entity.previous_position_x &&
            aux->entity.position_y == aux->entity.previous_position_y)
            aux->entity.runtime_flag_bits.contact_mask_b = 15;
    }
}

void FieldAuxiliary_UpdateRendererAtOffset(FieldAuxiliaryEntity *aux, u8 default_priority)
{
    if (aux->entity.base_state_flag_bits.animation_wait_enabled &&
        aux->entity.render_object->state_flag_bits.animation_active) {
        aux->entity.position_x += aux->offset_x;
        aux->entity.position_y += aux->offset_y;
        aux->entity.position_z += aux->offset_z;
        func_ov000_020a9d1c(&aux->entity, default_priority);
        aux->entity.position_x -= aux->offset_x;
        aux->entity.position_y -= aux->offset_y;
        aux->entity.position_z -= aux->offset_z;
    }
}

void FieldAuxiliary_UpdateAnimation(FieldAuxiliaryEntity *aux, int mode, u8 update_bounds)
{
    unsigned int animation;
    if (aux->entity.base_state_flag_bits.animation_wait_enabled &&
        aux->entity.render_object->state_flag_bits.animation_active) {
        if (aux->entity.locomotion_state <= 3) {
            func_ov000_020a4a1c(&aux->entity, mode, update_bounds);
            return;
        }
        if (aux->entity.locomotion_state == 96)
            return;
        const FieldAuxiliaryAnimationInfo *resource =
            *(const FieldAuxiliaryAnimationInfo **)aux->entity.unknown_1f0;
        if (!resource->flags.direction_mode)
            animation = aux->entity.animation_id;
        else
            animation = aux->entity.base_state_flag_bits.facing_direction;
        switch (aux->entity.locomotion_state) {
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
        case 25:
            animation += 8;
            break;
        case 26:
            animation += 16;
            break;
        case 27:
            break;
        case 76:
            break;
        case 77:
            animation += 8;
            break;
        case 94:
            break;
        case 95:
            animation += 8;
            break;
        }
        if (aux->entity.saved_presentation_flag_bits.unknown_14 || aux->entity.animation_id != animation) {
            aux->entity.render_object->set_animation((u8)animation, -1, 1);
            aux->entity.animation_id = animation;
            aux->entity.saved_presentation_flag_bits.unknown_14 = 0;
        }
        aux->entity.render_object->state_flag_bits.animation_suppressed = 0;
    }
}

void FieldAuxiliary_UpdateRenderPriority(FieldAuxiliaryEntity *aux)
{
    if (aux->entity.base_state_flag_bits.animation_wait_enabled &&
        aux->entity.render_object->state_flag_bits.animation_active && aux->target) {
        int mode = aux->bits.direction_mode;
        if (mode == 1)
            mode = data_ov000_020c0b78[aux->entity.base_state_flag_bits.facing_direction];
        switch (mode) {
        case 2: {
            unsigned int priority =
                aux->target->render_object->overlap_priority_bytes[aux->bits.unknown_05_06] <
                        aux->target->render_object->overlap_priority_bytes[aux->bits.unknown_07_08]
                    ? aux->target->render_object->overlap_priority_bytes[aux->bits.unknown_05_06]
                    : aux->target->render_object->overlap_priority_bytes[aux->bits.unknown_07_08];
            if (aux->bits.unknown_04) {
                aux->entity.render_object->overlap_priority_bytes[1] = priority;
                aux->entity.render_object->overlap_priority_bytes[0] =
                    aux->entity.render_object->overlap_priority_bytes[1];
            } else {
                aux->entity.render_object->overlap_priority_bytes[0] = priority;
                aux->entity.render_object->overlap_priority_bytes[1] = priority;
            }
            if (aux->entity.render_object->sort_key >= aux->target->render_object->sort_key)
                aux->entity.render_object->sort_key = aux->target->render_object->sort_key - 1;
            break;
        }
        case 3: {
            int priority = aux->target->render_object->overlap_priority_bytes[aux->bits.unknown_05_06] >
                                   aux->target->render_object->overlap_priority_bytes[aux->bits.unknown_07_08]
                               ? aux->target->render_object->overlap_priority_bytes[aux->bits.unknown_05_06]
                               : aux->target->render_object->overlap_priority_bytes[aux->bits.unknown_07_08];
            if (aux->bits.unknown_04) {
                aux->entity.render_object->overlap_priority_bytes[1] = priority;
                aux->entity.render_object->overlap_priority_bytes[0] =
                    aux->entity.render_object->overlap_priority_bytes[1];
            } else {
                if (aux->entity.render_object->overlap_priority_bytes[0] < priority)
                    aux->entity.render_object->overlap_priority_bytes[0] = priority;
                if (aux->entity.render_object->overlap_priority_bytes[1] < priority)
                    aux->entity.render_object->overlap_priority_bytes[1] = priority;
            }
            aux->entity.render_object->sort_key = aux->target->render_object->sort_key + 1;
            break;
        }
        default:
            if (aux->bits.unknown_04) {
                unsigned int priority =
                    aux->target->render_object->overlap_priority_bytes[aux->bits.unknown_05_06] >
                            aux->target->render_object->overlap_priority_bytes[aux->bits.unknown_07_08]
                        ? aux->target->render_object->overlap_priority_bytes[aux->bits.unknown_05_06]
                        : aux->target->render_object->overlap_priority_bytes[aux->bits.unknown_07_08];
                aux->entity.render_object->overlap_priority_bytes[1] = priority;
                aux->entity.render_object->overlap_priority_bytes[0] =
                    aux->entity.render_object->overlap_priority_bytes[1];
            }
        }
    }
}
}

extern "C" {

#include <game/field_party.h>
FieldPartyEntity *FieldPartyAuxiliary_CopyState(FieldPartyEntity *entity, const FieldPartyEntity *source)
{
    entity->auxiliary_bits.unknown_00 = source->auxiliary_bits.unknown_00;
    entity->auxiliary_bits.unknown_01 = source->auxiliary_bits.unknown_01;
    entity->auxiliary_bits.direction_mode = source->auxiliary_bits.direction_mode;
    entity->auxiliary_bits.unknown_04 = source->auxiliary_bits.unknown_04;
    entity->auxiliary_bits.unknown_05_06 = source->auxiliary_bits.unknown_05_06;
    entity->auxiliary_bits.unknown_07_08 = source->auxiliary_bits.unknown_07_08;
    entity->auxiliary_bits.unknown_09 = source->auxiliary_bits.unknown_09;
    entity->auxiliary_bits.unknown_10_15 = source->auxiliary_bits.unknown_10_15;
    entity->auxiliary_bits.unknown_16_18 = source->auxiliary_bits.unknown_16_18;
    entity->auxiliary_bits.unknown_19 = source->auxiliary_bits.unknown_19;
    entity->auxiliary_bits.unknown_20_27 = source->auxiliary_bits.unknown_20_27;
    entity->auxiliary_bits.unknown_28_31 = source->auxiliary_bits.unknown_28_31;
    entity->auxiliary_motion.unknown_524[0] = source->auxiliary_motion.unknown_524[0];
    entity->auxiliary_motion.unknown_524[1] = source->auxiliary_motion.unknown_524[1];
    entity->auxiliary_motion.unknown_524[2] = source->auxiliary_motion.unknown_524[2];
    entity->auxiliary_motion.unknown_524[3] = source->auxiliary_motion.unknown_524[3];
    entity->auxiliary_motion.unknown_524[4] = source->auxiliary_motion.unknown_524[4];
    entity->auxiliary_motion.unknown_524[5] = source->auxiliary_motion.unknown_524[5];
    entity->auxiliary_motion.unknown_53c = source->auxiliary_motion.unknown_53c;
    entity->auxiliary_motion.unknown_540 = source->auxiliary_motion.unknown_540;
    entity->auxiliary_motion.unknown_544 = source->auxiliary_motion.unknown_544;
    entity->auxiliary_motion.target = source->auxiliary_motion.target;
    FieldEntity_CopySpatialState(&entity->entity, &source->entity);
    return entity;
}

}
