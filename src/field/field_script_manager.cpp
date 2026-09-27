/*
 * Field script manager (overlay 0, 0x0208911C-0x020894B4).
 *
 * Initializes the two VM banks, starts room/entity/auxiliary scripts, and copies
 * the manager's state for a snapshot.
 */

#include <game/field_script_context.h>
extern "C" {
extern FieldScriptManager *data_ov000_020c1660[];
extern const u32 data_ov000_020c00d0[];
extern void func_0202cbd4(void *, int, u32);
extern void func_0202cc58(const void *, void *, u32);
extern void func_0202cd2c(const void *, void *, u32);
extern int func_ov000_02088008(FieldVmRuntime *, FieldScriptState *);
static inline void FieldScript_InitVm(FieldVmRuntime *runtime, const u8 *scripts, u32 variables)
{
    runtime->vm.unknown_00 = (u32)scripts;
    runtime->vm.unknown_04 = variables;
    runtime->vm.command_descriptors = data_ov000_020c00d0;
    runtime->vm.command_handler = FieldVm_DispatchCommand;
}

FieldScriptManager *FieldScriptManager_Construct(FieldScriptManager *manager)
{
    func_0202cbd4(manager->runtimes, 0, sizeof(manager->runtimes));
    return manager;
}

void FieldScriptManager_Init(FieldScriptManager *manager, void *field_context, int slot)
{
    FieldScriptContext *field = (FieldScriptContext *)field_context;
    const u16 *script;
    func_0202cbd4(manager->states, 0, sizeof(manager->states));
    data_ov000_020c1660[field->screen] = manager;
    FieldScript_InitVm(&manager->runtimes[0], field->scripts[0], field->variables);
    FieldScript_InitVm(&manager->runtimes[1], field->scripts[1], field->variables);
    manager->runtimes[0].field_context = (u8 *)field;
    manager->runtimes[1].field_context = (u8 *)field;
    manager->runtimes[0].screen_selector_bits.field_screen = 0;
    manager->runtimes[1].screen_selector_bits.field_screen = 1;
    manager->states[0].flag_bits.owner_type = 0;
    manager->states[1].flag_bits.owner_type = 1;
    manager->states[2].flag_bits.owner_type = 2;
    manager->states[3].flag_bits.owner_type = 3;
    manager->states[0].flag_bits.enabled = 1;
    manager->states[1].flag_bits.enabled = 1;
    manager->states[2].flag_bits.enabled = 0;
    manager->states[3].flag_bits.enabled = 1;
    manager->current_actor = -1;
    if (slot == -1)
        script = FieldScript_Lookup(field, 0, *field->area_header);
    else
        script = FieldScript_Lookup(field, 0, slot);
    FieldScript_Begin(&manager->states[0], 0, 3, script);
}

void FieldScriptManager_Destroy(FieldScriptManager *manager) {}

FieldScriptManager *FieldScriptManager_Copy(FieldScriptManager *destination, const FieldScriptManager *source)
{
    destination->current_actor = source->current_actor;
    if (source->runtimes < destination->runtimes)
        func_0202cd2c(source->runtimes, destination->runtimes, sizeof(destination->runtimes));
    else
        func_0202cc58(source->runtimes, destination->runtimes, sizeof(destination->runtimes));
    if (source->states < destination->states)
        func_0202cd2c(source->states, destination->states, sizeof(destination->states));
    else
        func_0202cc58(source->states, destination->states, sizeof(destination->states));
    return destination;
}

/* Start each eligible entity once, then run it immediately. The count is
 * cached, while entity slots and script fields remain live between calls. */
void FieldScriptManager_StartEntityScripts(FieldScriptManager *manager)
{
    FieldScriptContext *field = (FieldScriptContext *)manager->runtimes[0].field_context;
    FieldEntity **cursor = field->entities;
    int remaining = field->entity_count;
    if (!remaining)
        return;

    do {
        FieldEntity *entity = *cursor;
        if (entity->startup_script != -1 &&
            (entity->script_suppression_variable == 0xFFFF ||
             VM_ReadVariable(entity->script_suppression_variable, 0, 0) == 0)) {
            FieldScript_Begin((FieldScriptState *)entity->state_payload, 0, 3,
                             FieldScript_Lookup(field, entity->property_00a_bits.resource_set,
                                                entity->startup_script));
            func_ov000_02088008(&manager->runtimes[entity->property_00a_bits.resource_set],
                              (FieldScriptState *)entity->state_payload);
        }
        --remaining;
        ++cursor;
    } while (remaining);
}
/* The auxiliary slot may start only after its previous script has finished. */
void FieldScriptManager_StartAuxiliary(FieldScriptManager *manager, FieldScriptState *parent, int slot)
{
    if (!manager->states[3].flag_bits.active) {
        FieldScriptContext *field = (FieldScriptContext *)manager->runtimes[0].field_context;
        FieldScript_Begin(&manager->states[3], parent, 0, FieldScript_Lookup(field, 0, slot));
    }
}
}
