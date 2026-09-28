/* Destroy a timed field renderer and release its heap allocation. */
#include <game/field_timed_renderer.h>
#include <game/battle_scene.h>
extern "C" {
#include <game/heap.h>
extern FieldRenderObjectVTable data_ov000_020c14d4;
extern FieldRenderObjectVTable data_ov000_020c1594;
FieldTimedRenderer *FieldTimedRenderer_Delete(FieldTimedRenderer *model)
{
    *(FieldRenderObjectVTable **)model = &data_ov000_020c14d4;
    *(FieldRenderObjectVTable **)model = &data_ov000_020c1594;
    BattleModelController_DestroyBase((BattleModel *)model);
    GameHeap_Delete(model);
    return model;
}
}
