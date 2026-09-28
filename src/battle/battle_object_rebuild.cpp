/*
 * Object rebuild (overlay 2, 0x020890B4-0x02089300).
 *
 * Rebuilds an object's components one per frame after its data has been
 * reloaded.
 */

#include <game/battle_context.h>
#include <game/battle_scene.h>
#include <game/battle_object.h>
#include <game/battle_task_queue.h>

extern "C" {
extern u8 data_ov002_020bf6fc[];
extern int data_ov002_020c0c40;

void func_0202cbd4(void *destination, int value, u32 size);
void *GameHeap_New(u32 size, u32 heap_id, void *allocator, int argument);
void func_ov002_0206f1f0(BattleResourceModel *model,
                         BattleSceneResource *resource,
                         u32 component_offset);
void BattleTaskQueue_Promote(BattleQueuedTask *task);
}

void BattleObjectData_BeginRebuildTask(BattleQueuedTask *task) {
    BattleSceneResource *source = (BattleSceneResource *)task->argument;
    BattleSceneResource *destination =
        *((BattleSceneResource **)source - 1);
    BattleResourceModel *model =
        ((BattleContext *)gBattleContext)
            ->resource_models[destination->object_data_id];
    u32 component_count;

    if (model != 0) {
        /* The original path repeats this guard before virtual destruction. */
        if (model != 0) {
            model->destroy();
        }
    }

    model = (BattleResourceModel *)GameHeap_New(
        sizeof(BattleResourceModel),
        ((BattleContext *)gBattleContext)->runtime.resource_heap_id,
        data_ov002_020bf6fc, 0);
    if (model != 0) {
        model = (BattleResourceModel *)BattleRenderModel_Init((BattleModel *)model);
    }
    ((BattleContext *)gBattleContext)
        ->resource_models[destination->object_data_id] = model;

    model = ((BattleContext *)gBattleContext)
                ->resource_models[destination->object_data_id];
    func_ov002_0206f1f0(
        model, source, (source->flags.bits.resource_index - 1) << 6);
    model->finish();

    component_count = *(u16 *)(model->metadata + 12);
    data_ov002_020c0c40 = component_count;
    destination->component_14 =
        destination->data + component_count * sizeof(u32) + sizeof(u32);
    *(void **)destination->data = destination->component_14;
    model->component_index = 0;
    task->callback = BattleObjectData_RebuildNextComponentTask;
    BattleTaskQueue_Promote(task);
}

void BattleObjectData_RebuildNextComponentTask(BattleQueuedTask *task) {
    BattleSceneResource *source = (BattleSceneResource *)task->argument;
    BattleSceneResource *destination =
        *((BattleSceneResource **)source - 1);
    BattleResourceModel *model =
        ((BattleContext *)gBattleContext)
            ->resource_models[destination->object_data_id];
    BattleResourceStream *stream =
        &((BattleContext *)gBattleContext)->resource_stream;
    u8 *component = ((u8 **)destination->data)[model->component_index];
    u8 *next_component;

    GxCommandList_Begin(stream, component, 0x80000);
    model->write_component(stream);
    next_component = component + GxCommandList_End(stream);
    ++model->component_index;
    ((u8 **)destination->data)[model->component_index] = next_component;

    if (data_ov002_020c0c40 > model->component_index) {
        return;
    }

    destination->flags.raw &= ~(1 << 30);
    source->flags.raw &= ~(1 << 30);
    component = ((u8 **)destination->data)[model->component_index];
    if (destination->allocation_size -
            (u32)(component - destination->data) >
        100) {
        func_0202cbd4(component, 0, 100);
        destination->stream_state = component;
        *(u8 **)destination->stream_state = component + 100;
    } else {
        destination->stream_state = 0;
    }
    task->callback = 0;
}
