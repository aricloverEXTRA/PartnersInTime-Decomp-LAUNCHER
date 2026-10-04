/*
 * Battle script scheduler (overlay 2, 0x0207E7A0-0x0207F080).
 *
 * Owns the four task pools - action, reaction, auxiliary and object - and steps
 * every scheduled script once per frame. The Start* functions are how the rest
 * of the battle asks for a script to run: they take a task out of the right
 * pool, initialize its state from the script block, and link it in.
 *
 * HandleVmResult is where a script's return value is acted on: finish and
 * release the task, chain the continuation script it queued, or leave it for the
 * next frame. The party VM slots are the four fixed states the party members'
 * scripts run in.
 */

#include <game/battle_ai.h>

extern u32 func_ov002_020698cc(ScriptVm *vm, ScriptVmState *state);

void BattleAI_TaskPoolsInit(void) {
    BattleTaskPool_Init(&gBattleAIActionTaskPool, 8, 8);
    BattleTaskPool_Init(&gBattleAIReactionTaskPool, 8, 8);
    BattleTaskPool_Init(&gBattleAIAuxTaskPool, 8, 8);
    BattleTaskPool_Init(&gBattleAIObjectTaskPool, 40, 8);
}

void BattleAI_UpdateAll(void) {
    u8 *context = gBattleContext;
    BattleAIState *state;

    if (context == 0) {
        return;
    }

    state = (BattleAIState *)(context + BATTLE_AI_PARTY_STATE_1_OFFSET);
    if (state->script != 0) {
        VM_Run((ScriptVm *)(context + BATTLE_AI_VM_OFFSET),
               (ScriptVmState *)state);
    }

    context = gBattleContext;
    func_ov002_020698cc(
        (ScriptVm *)(context + BATTLE_AI_VM_OFFSET),
        (ScriptVmState *)(context + BATTLE_AI_PARTY_STATE_2_OFFSET)
    );

    context = gBattleContext;
    state = (BattleAIState *)(context + BATTLE_AI_PARTY_STATE_3_OFFSET);
    if (state->script != 0) {
        VM_Run((ScriptVm *)(context + BATTLE_AI_VM_OFFSET),
               (ScriptVmState *)state);
    }

    context = gBattleContext;
    state = (BattleAIState *)(context + BATTLE_AI_PARTY_STATE_4_OFFSET);
    if (state->script != 0) {
        VM_Run((ScriptVm *)(context + BATTLE_AI_VM_OFFSET),
               (ScriptVmState *)state);
    }

    BattleTaskList_Update(&gBattleAIActionTaskPool);
    BattleTaskList_Update(&gBattleAIReactionTaskPool);
    BattleTaskList_Update(&gBattleAIAuxTaskPool);
    BattleTaskList_Update(&gBattleAIObjectTaskPool);
}

void BattleAI_StartScriptById(int script_id) {
    int packed_id = script_id;
    int task_type;

    switch (packed_id) {
    case 1:
        BattleAI_StartPartyVmSlot1();
        return;
    case 2:
        BattleAI_StartPartyVmSlot2();
        return;
    case 3:
        BattleAI_StartPartyVmSlot3();
        return;
    case 4:
        BattleAI_StartPartyVmSlot4();
        return;
    }

    task_type = packed_id & BATTLE_AI_TASK_TYPE_MASK;
    switch (task_type) {
    case BATTLE_AI_TASK_ACTION:
        BattleAI_StartActionScript(
            packed_id & BATTLE_AI_TASK_ACTOR_ID_MASK
        );
        return;
    case BATTLE_AI_TASK_REACTION:
        BattleAI_StartReactionScript(
            packed_id & BATTLE_AI_TASK_ACTOR_ID_MASK
        );
        return;
    }
}

#include <game/battle_actor.h>

extern void func_0202cbd4(void *destination, int value, u32 size);

enum BattleAIPartyStateOffset {
    BATTLE_AI_PARTY_SLOT_1_SCRIPT_OFFSET = 0xD3BC,
    BATTLE_AI_PARTY_SLOT_3_SCRIPT_OFFSET = 0xD3C4,
    BATTLE_AI_PARTY_SLOT_4_SCRIPT_OFFSET = 0x35608
};
enum BattleAIActorTableOffset {
    BATTLE_AI_ENEMY_ACTOR_TABLE_OFFSET = 0x6508
};
typedef struct BattleAIScriptResource {
    u32 unknown_00;
    const void *script;
} BattleAIScriptResource;

typedef struct BattleAIEnemyActor {
    BattleActor actor;
    u8 action_state[0xB8];
    u8 reaction_state[0xB8];
    BattleAIState auxiliary_state;
} BattleAIEnemyActor;

void BattleAI_InitStateFromScriptBlock(BattleAIState *state,
                                       const u16 *script_block, int owner_id) {
    func_0202cbd4(state, 0, 0xB8);
    state->script = script_block + ((u32)*script_block >> 1);
    state->owner_id = owner_id;
}

void BattleAI_StartScriptTask(u16 actor_id, int task_type) {
    BattleAIEnemyActor *actor =
        *(BattleAIEnemyActor **)(gBattleContext +
                                 BATTLE_AI_ENEMY_ACTOR_TABLE_OFFSET +
                                 (s16)(actor_id - BATTLE_ACTOR_ENEMY_FIRST) * 4);
    BattleAIScriptResource *resource =
        (BattleAIScriptResource *)actor->actor.resource_slot;
    BattleAIState *state;
    void (*callback)(BattleAITask *task);
    BattleTaskPool *pool;
    BattleAITask *task;

    if (task_type == BATTLE_AI_TASK_ACTION) {
        state = (BattleAIState *)actor->action_state;
        pool = &gBattleAIActionTaskPool;
        callback = BattleAI_UpdateActionTask;
    } else if (task_type == BATTLE_AI_TASK_REACTION) {
        pool = &gBattleAIReactionTaskPool;
        callback = BattleAI_UpdateReactionTask;
        state = (BattleAIState *)actor->reaction_state;
    }

    func_0202cbd4(state, 0, 0xB8);
    state->script = resource->script;
    state->owner_id = task_type | actor_id;

    task = BattleAITask_GetOrInsert(pool, actor_id);
    task->callback = callback;
    task->state = state;
    task->id = actor_id;
}

void BattleAI_StartPartyVmSlot1(void) {
    BattleAI_InitStateFromScriptBlock(
        (BattleAIState *)(gBattleContext +
                          BATTLE_AI_PARTY_STATE_1_OFFSET),
        *(const u16 **)(gBattleContext +
                        BATTLE_AI_PARTY_SLOT_1_SCRIPT_OFFSET),
        1);
}

void BattleAI_StartPartyVmSlot2(void) {
}

void BattleAI_StartPartyVmSlot3(void) {
    BattleAI_InitStateFromScriptBlock(
        (BattleAIState *)(gBattleContext +
                          BATTLE_AI_PARTY_STATE_3_OFFSET),
        *(const u16 **)(gBattleContext +
                        BATTLE_AI_PARTY_SLOT_3_SCRIPT_OFFSET),
        3);
}

void BattleAI_StartPartyVmSlot4(void) {
    BattleAI_InitStateFromScriptBlock(
        (BattleAIState *)(gBattleContext +
                          BATTLE_AI_PARTY_STATE_4_OFFSET),
        (const u16 *)(gBattleContext +
                      BATTLE_AI_PARTY_SLOT_4_SCRIPT_OFFSET),
        4);
}

void BattleAI_StartActionScript(int actor_id) {
    BattleAI_StartScriptTask((u16)actor_id, BATTLE_AI_TASK_ACTION);
}

void BattleAI_StartReactionScript(int actor_id) {
    BattleAI_StartScriptTask((u16)actor_id, BATTLE_AI_TASK_REACTION);
}

void BattleAI_StartActorAuxScript(u16 actor_id, int target_actor_id,
                                  const void *script) {
    BattleAIEnemyActor *actor =
        *(BattleAIEnemyActor **)(gBattleContext +
                                 BATTLE_AI_ENEMY_ACTOR_TABLE_OFFSET +
                                 (s16)(actor_id - BATTLE_ACTOR_ENEMY_FIRST) * 4);
    BattleAIState *state = &actor->auxiliary_state;
    BattleAITask *task;

    actor->actor.target_actor_id = target_actor_id;
    state->script = script;
    state->scratch_a8 = 0;
    state->scratch_aa = 0;
    state->disabled = 0;
    state->order = 0;
    state->owner_id = BATTLE_AI_TASK_AUXILIARY | actor_id;

    task = BattleAITask_GetOrInsert(&gBattleAIAuxTaskPool, actor_id);
    task->callback = BattleAI_UpdateAuxTask;
    task->state = state;
    task->id = actor_id;
}

void BattleAI_StartObjectScript(int object_id, const void *script,
                                BattleAIState *source_state,
                                int queue_continuation) {
    BattleAIState *state = BattleScriptState_GetByObjectId((u16)object_id);
    BattleAITask *task;

    if (state->script != 0 && queue_continuation) {
        if (source_state->owner_id < BATTLE_AI_TASK_OBJECT) {
            state->continuation_order = source_state->owner_id;
            state->continuation_tie_break = 0xFFFF;
        } else {
            state->continuation_order = source_state->order_value;
            state->continuation_tie_break =
                source_state->owner_id & BATTLE_AI_TASK_ACTOR_ID_MASK;
        }
        state->continuation_script = script;
    } else {
        if (source_state->owner_id < BATTLE_AI_TASK_OBJECT) {
            state->order = source_state->owner_id;
            state->order_tie_break = 0xFFFF;
        } else {
            state->order = source_state->order_value;
            state->order_tie_break =
                source_state->owner_id & BATTLE_AI_TASK_ACTOR_ID_MASK;
        }
        state->script = script;
        state->scratch_a8 = 0;
        state->scratch_aa = 0;
        state->disabled = 0;
        state->continuation_script = 0;
        state->continuation_order = 0;
        state->continuation_tie_break = 0xFFFF;
    }

    state->owner_id = BATTLE_AI_TASK_OBJECT | object_id;
    task = BattleAITask_GetOrInsert(&gBattleAIObjectTaskPool, object_id);
    task->callback = BattleAI_UpdateChainedTask;
    task->state = state;
    task->id = object_id;
}

BattleAITask *BattleAITask_GetOrInsert(BattleTaskPool *pool, int actor_id) {
    BattleAITask *task = pool->active;
    BattleAITask *next;

    if (task != 0) {
        if (task->id > actor_id) {
            return BattleTaskList_Insert(pool, 0);
        }
        if (task->id == actor_id) {
            return task;
        }

        for (next = task->next; next != 0; next = next->next) {
            if (next->id > actor_id) {
                BattleAITask *inserted = BattleTaskPool_Allocate(pool);

                inserted->next = task->next;
                task->next = inserted;
                return task->next;
            }
            if (next->id == actor_id) {
                return next;
            }
            task = next;
        }

        next = BattleTaskPool_Allocate(pool);
        next->next = 0;
        task->next = next;
        return task->next;
    }

    return BattleTaskList_Insert(pool, 0);
}

void BattleAI_UpdateActionTask(BattleAITask *task) {
    BattleAIState *state = task->state;
    int result;

    if (state->disabled) {
        return;
    }
    if (state->wait_for_order && !BattleAI_TryClearOrderWait(state)) {
        return;
    }

    result = VM_Run(
        (ScriptVm *)(gBattleContext + BATTLE_AI_VM_OFFSET),
        (ScriptVmState *)state
    );
    switch (result) {
    case BATTLE_AI_VM_FINISHED:
    case BATTLE_AI_VM_CHAIN_SCRIPT:
        task->callback = 0;
        break;
    }
}

void BattleAI_UpdateReactionTask(BattleAITask *task) {
    BattleAIState *state = task->state;
    int result;

    if (state->disabled) {
        return;
    }
    if (state->wait_for_order && !BattleAI_TryClearOrderWait(state)) {
        return;
    }

    result = VM_Run(
        (ScriptVm *)(gBattleContext + BATTLE_AI_VM_OFFSET),
        (ScriptVmState *)state
    );
    switch (result) {
    case BATTLE_AI_VM_FINISHED:
    case BATTLE_AI_VM_CHAIN_SCRIPT:
        task->callback = 0;
        break;
    }
}

void BattleAI_UpdateAuxTask(BattleAITask *task) {
    BattleAIState *state = task->state;
    int result;

    if (state->disabled) {
        return;
    }

    result = VM_Run(
        (ScriptVm *)(gBattleContext + BATTLE_AI_VM_OFFSET),
        (ScriptVmState *)state
    );
    switch (result) {
    case BATTLE_AI_VM_FINISHED:
    case BATTLE_AI_VM_CHAIN_SCRIPT:
        task->callback = 0;
        break;
    }
}

void BattleAI_UpdateChainedTask(BattleAITask *task) {
    BattleAIState *state = task->state;

    if (state->script == 0) {
        task->callback = 0;
        state->order = 0;
        state->order_tie_break = 0xFFFF;
        state->continuation_script = 0;
        state->continuation_order = 0;
        state->continuation_tie_break = 0xFFFF;
        return;
    }
    if (state->disabled) {
        return;
    }

    do {
        int result = VM_Run(
            (ScriptVm *)(gBattleContext + BATTLE_AI_VM_OFFSET),
            (ScriptVmState *)state
        );

        if (!BattleAI_HandleVmResult(task, result, state)) {
            return;
        }
    } while (1);
}

int BattleAI_HandleVmResult(BattleAITask *task, int result, BattleAIState *state) {
    switch (result) {
    case BATTLE_AI_VM_FINISHED:
        goto finish;
    case BATTLE_AI_VM_CHAIN_SCRIPT:
        if (state->continuation_script != 0) {
            state->script = state->continuation_script;
            state->order = state->continuation_order;
            state->order_tie_break = state->continuation_tie_break;
            state->continuation_script = 0;
            state->continuation_order = 0;
            state->continuation_tie_break = 0xFFFF;
            return 1;
        }
        goto finish;
    case BATTLE_AI_VM_YIELD:
        goto return_zero;
    }

    goto return_zero;

/* Shared exits preserve the original compiler's compact switch layout. */
finish:
    task->callback = 0;
    state->order = 0;
    state->order_tie_break = 0xFFFF;
    state->continuation_script = 0;
    state->continuation_order = 0;
    state->continuation_tie_break = 0xFFFF;

return_zero:
    return 0;
}
