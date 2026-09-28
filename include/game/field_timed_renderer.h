#ifndef PIT_GAME_FIELD_TIMED_RENDERER_H
#define PIT_GAME_FIELD_TIMED_RENDERER_H

/*
 * A renderer whose animation runs for a set number of loops, with the render list
 * it is drawn from.
 */

#include <game/field_entity.h>

struct ModelRenderDescriptor;
struct ModelRenderSortKey;

typedef struct FieldAnimationRenderer {
    FieldRenderObject base;
    union {
        u16 raw;
        struct { u16 unknown_00_03 : 4, finished : 1; s16 loops_remaining : 11; };
    } control;
    u16 unknown_13a;
} FieldAnimationRenderer;

typedef struct FieldTimedRenderer {
    FieldAnimationRenderer animation;
    union {
        u32 raw;
        struct { u32 unknown_00_14 : 15, expired : 1, unknown_16_31 : 16; } bits;
        struct { u16 flags, remaining; } halves;
    } state;
} FieldTimedRenderer;

typedef char FieldAnimationRenderer_SizeCheck[sizeof(FieldAnimationRenderer) == 0x13C ? 1 : -1];
typedef char FieldTimedRenderer_SizeCheck[sizeof(FieldTimedRenderer) == 0x140 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
FieldAnimationRenderer *FieldAnimationRenderer_Init(FieldAnimationRenderer *model);
FieldAnimationRenderer *FieldAnimationRenderer_InitBase(FieldAnimationRenderer *model);
FieldAnimationRenderer *FieldAnimationRenderer_DestroyBase(FieldAnimationRenderer *model);
FieldTimedRenderer *FieldTimedRenderer_DestroyBase(FieldTimedRenderer *model);
FieldTimedRenderer *FieldTimedRenderer_Delete(FieldTimedRenderer *model);
void FieldAnimationRenderer_RestoreController(FieldRenderObject *model,
    const struct ModelRenderDescriptor *descriptor, void *controller, s16 animation);
u8 FieldAnimationRenderer_GetOverlapPriority(const FieldRenderObject *model,
    const struct ModelRenderSortKey *key);
void FieldRenderList_Clear(int screen);
void FieldTimedRenderer_Update(FieldTimedRenderer *model);
void FieldTimedRenderer_UpdateAnimation(FieldAnimationRenderer *model);
void FieldTimedRenderer_ResetAnimationControl(FieldAnimationRenderer *model);
void FieldTimedRenderer_SetLoopCount(FieldAnimationRenderer *model, int loop_count);
void FieldTimedRenderer_RestartAnimation(FieldAnimationRenderer *model);
#ifdef __cplusplus
}
#endif

#endif
