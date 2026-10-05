#ifndef PIT_GAME_FIELD_ROAMING_H
#define PIT_GAME_FIELD_ROAMING_H
#include <game/field_entity.h>
/* The script stores five full-width words; delay and direction are narrowed
 * when copied into an option slot. Bounds use whole pixels. */
typedef struct FieldRoamingOptionInput {
    int argument;
    fx32 speed;
    int distance,delay,direction_mode;
} FieldRoamingOptionInput;
typedef char FieldRoamingOptionInput_SizeCheck[sizeof(FieldRoamingOptionInput)==20?1:-1];
#ifdef __cplusplus
extern "C" {
#endif
void FieldEntity_SelectBodyContact(FieldRuntimeEntity *entity, FieldRuntimeEntity *contact, u8 mask);
/* Group bits map to native flags 0, 2/3, 1, 6 and 4, respectively.
 * Individual flag behavior remains unnamed pending runtime evidence. */
void FieldEntity_SetCollisionFlagGroups(FieldRuntimeEntity *entity, u8 mask, u8 enabled);
/* A value of -1 preserves the corresponding group. */
void FieldEntity_UpdateCollisionFlagGroups(FieldRuntimeEntity *entity, int flag0, int flag23, int flag1,
                                           int flag6, int flag4);
void FieldEntity_LoadBodyBounds(FieldRuntimeEntity *entity);
/* Bits 0/1/2 report X/Y/height overlap, respectively. */
int FieldEntity_GetBodyOverlapMask(FieldRuntimeEntity *entity, FieldRuntimeEntity *other);
void FieldRoaming_ClearOptions(FieldRuntimeEntity *entity,int index);
int FieldRoaming_AddOption(FieldRuntimeEntity *entity,const FieldRoamingOptionInput *input,int index);
void FieldRoaming_SetBounds(FieldRuntimeEntity *entity,int minimum_x,int minimum_y,int maximum_x,int maximum_y);
/* Chooses a bounded destination, trying at most eight random directions. */
void FieldRoaming_ChooseDestination(FieldRuntimeEntity *entity);
void FieldRoaming_Update(FieldRuntimeEntity *entity);
void FieldRoaming_Resume(FieldRuntimeEntity *entity);
void FieldRoaming_Pause(FieldRuntimeEntity *entity);
void FieldRoaming_Stop(FieldRuntimeEntity *entity);
void FieldPath_Advance(FieldRuntimeEntity *entity);
void FieldPath_Start(FieldRuntimeEntity *entity);
void FieldPath_Clear(FieldRuntimeEntity *entity);
/* Builds the path point table from a script list whose first two entries are
 * flags (entry 0 X = bounce, entry 1 X = random direction, entry 1 Y = delay).
 * A nonzero entry 0 Y accumulates the remaining Q12 offsets from the entity's
 * current position. */
void FieldPath_Setup(FieldRuntimeEntity *entity, const FieldPathPoint *list, int count);
#ifdef __cplusplus
}
#endif
#endif
