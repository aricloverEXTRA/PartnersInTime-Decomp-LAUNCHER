#ifndef PIT_GAME_FIELD_AUXILIARY_H
#define PIT_GAME_FIELD_AUXILIARY_H
#include <game/field_entity_lifecycle.h>
#include <game/field_entity_motion.h>
typedef struct FieldAuxiliaryEntity {
    FieldRuntimeEntity entity;
    union {
        u32 flags;
        struct {
            u32 follow_position : 1, follow_direction : 1, direction_mode : 2, unknown_04 : 1;
            u32 unknown_05_06 : 2, unknown_07_08 : 2, follow_locomotion : 1, unknown_10_15 : 6;
            u32 kind : 3, unknown_19 : 1;
            s32 resource_index : 8, unknown_28_31 : 4;
        } bits;
    };
    fx32 offset_x, offset_y, offset_z;
    fx32 previous_offset_x, previous_offset_y, previous_offset_z;
    fx32 owner_offset_x, owner_offset_y, owner_offset_z;
    FieldRuntimeEntity *owner, *target;
} FieldAuxiliaryEntity;
typedef char FieldAuxiliaryEntity_SizeCheck[sizeof(FieldAuxiliaryEntity) == 1360 ? 1 : -1];
/* Only the resource record's direction-selection bits are interpreted here. */
typedef struct FieldAuxiliaryAnimationInfo {
    u8 unknown_00[7];
    struct {
        u8 unknown_00 : 1, direction_mode : 2, unknown_03_07 : 5;
    } flags;
} FieldAuxiliaryAnimationInfo;
typedef char FieldAuxiliaryAnimationInfo_SizeCheck[sizeof(FieldAuxiliaryAnimationInfo) == 8 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
void FieldAuxiliary_StopSpecialRenderer(FieldAuxiliaryEntity *aux);
void FieldAuxiliary_UpdateRenderPriority(FieldAuxiliaryEntity *aux);
void FieldAuxiliary_UpdateAnimation(FieldAuxiliaryEntity *aux, int mode, u8 update_bounds);
void FieldAuxiliary_UpdateRendererAtOffset(FieldAuxiliaryEntity *aux, u8 default_priority);
void FieldAuxiliary_UpdateContactsAtOffset(FieldAuxiliaryEntity *aux);
void FieldAuxiliary_UpdateCollisionAtOffset(FieldAuxiliaryEntity *aux, int mode);
int FieldAuxiliary_TestNavigationAtOffset(FieldAuxiliaryEntity *aux, int mask, int mode);
void FieldAuxiliary_MapLocomotionState(FieldAuxiliaryEntity *aux);
void FieldAuxiliary_UpdateLocomotionState(FieldAuxiliaryEntity *aux);
void FieldAuxiliary_BeginFrame(FieldAuxiliaryEntity *aux);
void FieldAuxiliary_AllocateRenderer(FieldAuxiliaryEntity *aux);
FieldAuxiliaryEntity *FieldAuxiliary_Delete(FieldAuxiliaryEntity *aux);
FieldAuxiliaryEntity *FieldAuxiliary_Destroy(FieldAuxiliaryEntity *aux);
FieldAuxiliaryEntity *FieldAuxiliary_Init(FieldAuxiliaryEntity *aux);
FieldAuxiliaryEntity *FieldAuxiliary_InitPlacement(FieldAuxiliaryEntity *aux, int index, int argument,
                                                   int resource_set, int kind, FieldRuntimeEntity *owner);
#ifdef __cplusplus
}
#endif
#endif
