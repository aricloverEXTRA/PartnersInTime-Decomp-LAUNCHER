/*
 * Session task (ARM9 resident, 0x020062EC-0x020063A4).
 *
 * Init, Destroy and Delete for the task that owns the running session.
 */

#include <game/session.h>
#include <game/save_data.h>
#include <game/heap.h>

extern GameTaskVTable data_02050020;
extern void *data_02059f64;
extern void func_02004f9c(u32 language);

GameSessionTask *GameSessionTask_Init(GameSessionTask *task, u32 priority, u32 unused, void *argument,
                             u32 requested_state, u32 unknown, const GameSessionSettings *settings)
{
    GameTask_Init(&task->base, priority, unused, argument);
    task->base.vtable = &data_02050020;
    func_02004f9c(settings->language);
    GameSession_InitializeSaveContext(settings);
    *(void **)(gSaveData + 0x51c) = data_02059f64;
    GameSessionTask_RequestState(task, requested_state);
    return task;
}

GameSessionTask *GameSessionTask_Destroy(GameSessionTask *task)
{
    task->base.vtable = &data_02050020;
    GameTask_DestroyBase(&task->base);
    return task;
}

GameSessionTask *GameSessionTask_Delete(GameSessionTask *task)
{
    task->base.vtable = &data_02050020;
    GameTask_DestroyBase(&task->base);
    GameHeap_Delete(task);
    return task;
}
