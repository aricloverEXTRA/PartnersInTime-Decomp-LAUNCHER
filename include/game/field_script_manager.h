#ifndef PIT_GAME_FIELD_SCRIPT_MANAGER_H
#define PIT_GAME_FIELD_SCRIPT_MANAGER_H

/*
 * The manager that owns the field's script VM instance and its running scripts.
 */

#include <game/field_entity.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <game/field_script.h>
#ifdef __cplusplus
}
#endif

struct FieldPartyEntity;
struct FieldAreaContext;

typedef struct FieldScriptManager {
    /* Region-trigger selection reuses the original actor byte and stores the
     * party side in bit 8 while waiting for an action to finish. */
    union {
        struct { s8 current_actor; u8 reserved[3]; };
        s8 pending_region;
        struct { u32 unknown_00_07 : 8, party_side : 1, unknown_09_31 : 23; } region_selection;
    };
    FieldVmRuntime runtimes[2];
    FieldScriptState states[4];
} FieldScriptManager;

typedef char FieldScriptManager_SizeCheck[sizeof(FieldScriptManager) == 0x464 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
FieldScriptManager *FieldScriptManager_Construct(FieldScriptManager *manager);
void FieldScriptManager_Init(FieldScriptManager *manager, void *field_context, int slot);
void FieldScriptManager_StartEntityScripts(FieldScriptManager *manager);
void FieldScriptManager_Destroy(FieldScriptManager *manager);
FieldScriptManager *FieldScriptManager_Copy(FieldScriptManager *destination,
                                            const FieldScriptManager *source);
void FieldScriptManager_StartAuxiliary(FieldScriptManager *manager, FieldScriptState *parent, int slot);
/* Returns the script-state slot (1 or 2), or -1 if no region script starts. */
int FieldScriptManager_TryRegionScript(FieldScriptManager *manager,
    struct FieldPartyEntity *member, struct FieldAreaContext *area);
void FieldScript_BeginInteraction(FieldVmRuntime *runtime, FieldRuntimeEntity *actor,
                                  FieldRuntimeEntity *target, FieldScriptState *state);
void FieldScript_EndInteraction(FieldVmRuntime *runtime, FieldScriptState *state);
void FieldEntity_SetAnimationWait(FieldRuntimeEntity *entity, s8 enabled);
void FieldEntity_RestoreContactFlags(FieldRuntimeEntity *entity);
#ifdef __cplusplus
}
#endif

#endif
