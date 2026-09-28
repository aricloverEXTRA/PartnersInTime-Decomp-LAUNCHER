#ifndef PIT_GAME_FIELD_SCRIPT_H
#define PIT_GAME_FIELD_SCRIPT_H

/*
 * The field script's interface to the world: the entity and area state a command
 * may read or write, and the handles it addresses them by.
 */

#include <game/script_vm.h>

typedef struct FieldEntity FieldEntity;
typedef struct FieldScriptState FieldScriptState;
typedef struct FieldVmRuntime FieldVmRuntime;

typedef struct FieldVmScreenSelectorBits {
    u32 field_screen : 1;
    u32 unknown_01_31 : 31;
} FieldVmScreenSelectorBits;

enum FieldScriptFlag {
    FIELD_SCRIPT_ENABLED = 1 << 0,
    FIELD_SCRIPT_ACTIVE = 1 << 1,
    FIELD_SCRIPT_PAUSED = 1 << 2,
    FIELD_SCRIPT_OWNER_TYPE_SHIFT = 4,
    FIELD_SCRIPT_OWNER_TYPE_MASK = 7 << FIELD_SCRIPT_OWNER_TYPE_SHIFT,
    FIELD_SCRIPT_CONTEXT_TYPE_SHIFT = 7,
    FIELD_SCRIPT_CONTEXT_TYPE_MASK = 3 << FIELD_SCRIPT_CONTEXT_TYPE_SHIFT,
    FIELD_SCRIPT_PARENT_TYPE_SHIFT = 9,
    FIELD_SCRIPT_PARENT_TYPE_MASK = 7 << FIELD_SCRIPT_PARENT_TYPE_SHIFT,
    FIELD_SCRIPT_QUEUED_PARENT_TYPE_SHIFT = 12,
    FIELD_SCRIPT_QUEUED_PARENT_TYPE_MASK =
        7 << FIELD_SCRIPT_QUEUED_PARENT_TYPE_SHIFT,
    FIELD_SCRIPT_RESULT_SHIFT = 15,
    FIELD_SCRIPT_RESULT_MASK = 3 << FIELD_SCRIPT_RESULT_SHIFT,
    FIELD_SCRIPT_INLINE_WAIT = 1 << 17,
    FIELD_SCRIPT_OWNER_SUBTYPE_SHIFT = 18,
    FIELD_SCRIPT_OWNER_SUBTYPE_MASK = 0xFF << FIELD_SCRIPT_OWNER_SUBTYPE_SHIFT
};

enum FieldScriptOwnerType {
    FIELD_SCRIPT_OWNER_PRIMARY = 1,
    FIELD_SCRIPT_OWNER_SECONDARY = 2,
    FIELD_SCRIPT_OWNER_ENTITY = 4
};

typedef struct FieldScriptFlagBits {
    u32 enabled : 1;
    u32 active : 1;
    u32 paused : 1;
    u32 unknown_03 : 1;
    u32 owner_type : 3;
    u32 context_type : 2;
    u32 parent_type : 3;
    u32 queued_parent_type : 3;
    u32 result : 2;
    u32 inline_wait : 1;
    s32 owner_subtype : 8;
    u32 unknown_26_31 : 6;
} FieldScriptFlagBits;

struct FieldScriptState {
    ScriptVmState vm_state;
    u32 unknown_ac;
    union {
        u32 flags;
        FieldScriptFlagBits flag_bits;
    };
    u8 owner_data[4];
    s8 parent_entity_id;
    s8 queued_parent_entity_id;
    s16 script_values[2];
    u8 unknown_be[2];
    const u16 *queued_script;
    u32 unknown_c4;
    FieldEntity *owner_entity;
};

struct FieldVmRuntime {
    ScriptVm vm;
    u8 unknown_010[0x80];
    union {
        u32 screen_selector;
        FieldVmScreenSelectorBits screen_selector_bits;
    };
    u8 *field_context;
};

typedef char FieldScriptState_SizeCheck[
    sizeof(FieldScriptState) == 0xCC ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif

/* Fill save flags 1024..2047; any nonzero enabled value sets every bit. */
void FieldVm_FillExtendedSaveFlags(int enabled);
int FieldScript_Begin(FieldScriptState *state, FieldScriptState *parent,
                      int context_type, const u16 *script);
FieldEntity *FieldScript_ResolveEntity(FieldVmRuntime *runtime,
                                       FieldScriptState *state, int selector);
#ifdef __cplusplus
}
#endif

int FieldVm_DispatchCommand(ScriptVm *vm, ScriptVmState *state,
                            ScriptVmCommand *command);

#endif
