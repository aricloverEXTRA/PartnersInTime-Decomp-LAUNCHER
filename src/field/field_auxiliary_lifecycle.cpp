/*
 * Auxiliary entity lifecycle (overlay 0, 0x020B72E0-0x020B756C).
 *
 * An auxiliary entity is one attached at an offset to another - a carried baby,
 * a held object. This is its construction, its renderer allocation and its
 * per-frame locomotion state.
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

FieldAuxiliaryEntity *FieldAuxiliary_InitPlacement(FieldAuxiliaryEntity *aux, int index, int argument,
                                                   int resource_set, int kind, FieldRuntimeEntity *owner)
{
    FieldEntity3D_InitPlacement(&aux->entity, index, 0, argument, resource_set, 0);
    *(FieldEntityVTable **)aux = &data_ov000_020c1194;
    func_0202cbd4(&aux->flags, 0, 48);
    aux->bits.follow_position = 0;
    aux->bits.follow_direction = 0;
    aux->bits.follow_locomotion = 0;
    aux->owner = owner;
    aux->entity.base.property_00a_bits.subtype = 10;
    aux->entity.collision_state_flag_bits.unknown_18_19 = 1;
    aux->bits.kind = kind;
    aux->bits.unknown_19 = 0;
    aux->bits.resource_index = -1;
    aux->bits.unknown_28_31 = -1;
    aux->entity.contact_direction_flags.enabled_mask = 63;
    return aux;
}

FieldAuxiliaryEntity *FieldAuxiliary_Init(FieldAuxiliaryEntity *aux)
{
    FieldEntity3D_Init(&aux->entity);
    *(FieldEntityVTable **)aux = &data_ov000_020c1194;
    func_0202cbd4((u8 *)aux + 4, 0, 1356);
    return aux;
}

FieldAuxiliaryEntity *FieldAuxiliary_Destroy(FieldAuxiliaryEntity *aux)
{
    *(FieldEntityVTable **)aux = &data_ov000_020c1194;
    FieldEntity3D_DestroyBase(&aux->entity);
    return aux;
}

FieldAuxiliaryEntity *FieldAuxiliary_Delete(FieldAuxiliaryEntity *aux)
{
    *(FieldEntityVTable **)aux = &data_ov000_020c1194;
    FieldEntity3D_DestroyBase(&aux->entity);
    GameHeap_Delete(aux);
    return aux;
}

void FieldAuxiliary_AllocateRenderer(FieldAuxiliaryEntity *aux)
{
    FieldAnimationRenderer *renderer =
        (FieldAnimationRenderer *)GameHeap_New(316, aux->entity.base.property_00a_bits.heap, 0, 1);
    if (renderer)
        renderer = FieldAnimationRenderer_InitBase(renderer);
    aux->entity.render_object = (FieldRenderObject *)renderer;
    aux->entity.auxiliary_render_object = 0;
}

void FieldAuxiliary_BeginFrame(FieldAuxiliaryEntity *aux)
{
    FieldEntity3D_BeginFrame(&aux->entity);
    aux->previous_offset_x = aux->offset_x;
    aux->previous_offset_y = aux->offset_y;
    aux->previous_offset_z = aux->offset_z;
}

void FieldAuxiliary_UpdateLocomotionState(FieldAuxiliaryEntity *aux)
{
    if (aux->entity.base.property_00a_bits.property_00a_flag_00) {
        if (aux->bits.follow_locomotion)
            aux->entity.locomotion_state = aux->owner->locomotion_state;
        else if (aux->entity.locomotion_state <= 3)
            FieldEntity3D_UpdateLocomotionState(&aux->entity);
        aux->entity.base.map_locomotion_state();
    }
}

void FieldAuxiliary_MapLocomotionState(FieldAuxiliaryEntity *aux)
{
    if (aux->bits.follow_locomotion)
        aux->entity.locomotion_category = aux->owner->locomotion_category;
    else
        aux->entity.locomotion_category = data_ov000_020c0c00[aux->entity.locomotion_state];
}
}
