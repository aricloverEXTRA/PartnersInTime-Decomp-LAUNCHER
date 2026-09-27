/* Battle frame driver, overlay 2, 0x020729D4-0x02072D90. */
#include "battle_scheduler_internal.h"

#include <game/task.h>

extern "C" {
#include <game/archive_io.h>
#include <game/input.h>
#include <game/audio.h>
void func_02037108(void);
BattleTransferTask *BattleTransfer_EnqueueBeforeMapping(BattleTransferCallback, u32, u32, u32);
BattleTransferTask *BattleTransfer_EnqueueAfterMapping(BattleTransferCallback, u32, u32, u32);
int BattleScheduler_ElapsedScanlines(int);
}

#define GX_DIFFUSE_AMBIENT (*(vu32 *)0x040004C0)
#define GX_SPECULAR_EMISSION (*(vu32 *)0x040004C4)
#define GX_LIGHT_VECTOR (*(vu32 *)0x040004C8)
#define GX_LIGHT_COLOR (*(vu32 *)0x040004CC)
#define GX_SWAP_BUFFERS (*(vu32 *)0x04000540)

static inline int HasPendingOpen(ArchiveIO *archive)
{
    if (archive->open_head != archive->open_tail) return 1;
    return 0;
}

static inline int HasPendingRead(ArchiveIO *archive)
{
    if (archive->first) return 1;
    return 0;
}

static inline int HasPendingOverlay(ArchiveIO *archive)
{
    if (archive->overlay_state) return 1;
    return 0;
}

extern "C" void BattleScheduler_Update(ArchiveIO *archive)
{
    gBattleSystem->vblank_count = 0;
    GameInput_Read();
    func_02037108();
    GX_DIFFUSE_AMBIENT = 0x42107fff;
    GX_SPECULAR_EMISSION = 0;
    GX_LIGHT_COLOR = 0x7fff;
    GX_LIGHT_VECTOR = 0xa596a;
    /* Requeue transfers submitted from inside the previous VBlank. */
    for (int i = 0; i < 32; ++i) {
        BattleTransferTask *task = &gBattleSystem->deferred_before_mapping[i];
        if (!task->callback) break;
        BattleTransfer_EnqueueBeforeMapping(task->callback,
            task->argument_1, task->argument_2, task->argument_3);
        task->callback = 0;
    }
    for (int i = 0; i < 32; ++i) {
        BattleTransferTask *task = &gBattleSystem->deferred_after_mapping[i];
        if (!task->callback) break;
        BattleTransfer_EnqueueAfterMapping(task->callback,
            task->argument_1, task->argument_2, task->argument_3);
        task->callback = 0;
    }
    /* Read next after the callback: nodes may change the live list. */
    for (BattleSchedulerNode *node = gBattleSystem->first; node; node = node->next) {
        if (node->update) node->update();
    }
    if (!gBattleSystem->first) {
        if (archive) ((GameTaskDispatch *)archive)->delete_task();
        return;
    }
    if (gBattleSystem->vblank_count) gBattleSystem->vblank_count = 0;
    if (gBattleSystem->active_task.callback) {
        do {
            /* Leave at least 15 scanlines for the end of the visible frame. */
            if (192 - BattleScheduler_ElapsedScanlines(0) < 15) {
                GX_SWAP_BUFFERS = 1;
                gBattleSystem->flags.raw |= 2;
                GameFrame_WaitVBlank();
                return;
            }
            gBattleSystem->active_task.callback(&gBattleSystem->active_task);
        } while (gBattleSystem->active_task.callback);
    }
    GX_SWAP_BUFFERS = 1;
    gBattleSystem->flags.raw |= 2;
    /* VBlank can end this work loop between any two callbacks. */
    for (;;) {
        if (gBattleSystem->vblank_count) return;
        if (HasPendingOpen(archive)) {
            ArchiveIO_ProcessOpen(archive);
        } else if (HasPendingRead(archive)) {
            ArchiveIO_ProcessCompressedRead(archive);
        } else if (HasPendingOverlay(archive)) {
            ArchiveIO_ProcessOverlay(archive);
        } else if (GameAudio_IsLoading()) {
            GameAudio_ProcessLoading();
        } else {
            int head = gBattleSystem->task_head;
            if (head == gBattleSystem->task_tail) {
                GameFrame_WaitVBlank();
                return;
            }
            BattleQueuedTask *task = &gBattleSystem->tasks[head];
            if (task->callback) task->callback(task);
            if (gBattleSystem->flags.bits.active_task) {
                gBattleSystem->vblank_count = 0;
                gBattleSystem->flags.raw |= 2;
                GameFrame_WaitVBlank();
                return;
            }
            if (!task->callback) {
                if (++head == 32) gBattleSystem->task_head = 0;
                else gBattleSystem->task_head = head;
            }
        }
    }
}
