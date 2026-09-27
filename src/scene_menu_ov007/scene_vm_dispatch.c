/*
 * Scene script dispatcher (overlay 7, 0x02081730-0x02083B1C).
 *
 * The scene and object language's opcode table. Kept as one switch because its
 * layout is part of the match; opcode 0x04E contains a documented inline-assembly
 * height calculation. See docs/research/SCENE_VM_MATCHING.md before touching
 * either.
 */

#include <game/scene_sound.h>
#include <game/scene_vm_state.h>
#include <game/scene_motion.h>
#include <game/save_data.h>
#include <game/pause_scene.h>

/*
 * Scene/object command dispatcher (overlay 7, 0x02081730-0x02083B1C).
 *
 * This file deliberately keeps the original single-switch shape.  Splitting
 * commands into out-of-line handlers would make the code prettier today, but
 * would also destroy the compiler layout that the matching build ultimately
 * has to reproduce.  The named cases and small access macros keep the C
 * readable without changing that control-flow topology.
 */

typedef union SceneResourceFlags {
    u32 raw;
    struct {
        u32 render_group : 2;
        u32 unknown_2_31 : 30;
    } bits;
} SceneResourceFlags;

typedef struct SceneResource {
    u8 unknown_00[0x7C];
    SceneResourceFlags flags;
} SceneResource;

typedef struct SceneVmManagerLayout {
    u8 unknown_0000[0x39DC];
    SceneScriptState object_scripts[40];
    u8 unknown_57dc[0x5010];
    void *sound_tasks[16];
} SceneVmManagerLayout;

enum SceneVmOpcode {
    SCENE_OP_LOAD_OBJECT_RESOURCE = 0x035,
    SCENE_OP_WAIT_OBJECT_RESOURCE = 0x039,
    SCENE_OP_BIND_OBJECT_RESOURCE = 0x03A,
    SCENE_OP_ACTIVATE_OBJECT_RESOURCE = 0x03B,
    SCENE_OP_YIELD_03E = 0x03E,
    SCENE_OP_SWAP_OBJECT_SLOTS = 0x03F,
    SCENE_OP_YIELD_042 = 0x042,
    SCENE_OP_YIELD_045 = 0x045,
    SCENE_OP_GET_GLOBAL_PROPERTY = 0x046,
    SCENE_OP_SET_GLOBAL_PROPERTY = 0x047,
    SCENE_OP_GET_OBJECT_PROPERTY = 0x04B,
    SCENE_OP_SET_OBJECT_PROPERTY = 0x04C,
    SCENE_OP_SET_OBJECT_PROPERTY_FIXED = 0x04D,
    SCENE_OP_ALIGN_OBJECTS = 0x04E,
    SCENE_OP_SET_OBJECT_POSITION = 0x050,
    SCENE_OP_GET_OBJECT_COORDINATE = 0x051,
    SCENE_OP_WAIT_OBJECT_PENDING_STATE = 0x054,
    SCENE_OP_SET_OBJECT_ANIMATION = 0x055,
    SCENE_OP_MOVE_OBJECT = 0x056,
    SCENE_OP_MOVE_OBJECT_COMPLEX = 0x057,
    SCENE_OP_MOVE_OBJECT_AT_SPEED = 0x058,
    SCENE_OP_KINEMATIC_AXIS2_A = 0x059,
    SCENE_OP_KINEMATIC_TARGET_A = 0x05A,
    SCENE_OP_KINEMATIC_AXIS2_A_RESULT = 0x05B,
    SCENE_OP_KINEMATIC_TARGET_A_RESULT = 0x05C,
    SCENE_OP_KINEMATIC_AXIS2_B = 0x05D,
    SCENE_OP_KINEMATIC_TARGET_B = 0x05E,
    SCENE_OP_KINEMATIC_AXIS2_C = 0x05F,
    SCENE_OP_KINEMATIC_AXIS2_B_RESULT = 0x060,
    SCENE_OP_KINEMATIC_TARGET_B_RESULT = 0x061,
    SCENE_OP_KINEMATIC_AXIS2_C_RESULT = 0x062,
    SCENE_OP_BALLISTIC_TERMINAL_SPEED = 0x063,
    SCENE_OP_BALLISTIC_ACCELERATION = 0x064,
    SCENE_OP_BALLISTIC_ROOT = 0x065,
    SCENE_OP_PARAMETRIC_MOTION = 0x068,
    SCENE_OP_WAIT_OBJECT_MOTION = 0x069,
    SCENE_OP_CANCEL_OBJECT_MOTION = 0x06A,
    SCENE_OP_START_OBJECT_PATH = 0x06B,
    SCENE_OP_GET_OBJECT_MOTION_PROPERTY = 0x06C,
    SCENE_OP_SET_SCREEN_EFFECT = 0x06D,
    SCENE_OP_RESET_SCREEN_STATE = 0x06E,
    SCENE_OP_GET_SCENE_STATE = 0x06F,
    SCENE_OP_WAIT_SCENE_READY = 0x070,
    SCENE_OP_SET_SCENE_MODE = 0x071,
    SCENE_OP_SET_SCENE_FLAG = 0x072,
    SCENE_OP_GET_SCENE_RECORD_PROPERTY = 0x073,
    SCENE_OP_YIELD_079 = 0x079,
    SCENE_OP_YIELD_08D = 0x08D,
    SCENE_OP_YIELD_08E = 0x08E,
    SCENE_OP_YIELD_099 = 0x099,
    SCENE_OP_CLEAR_PERSISTENT_INPUT = 0x09F,
    SCENE_OP_INJECT_INPUT_MASK = 0x0A0,
    SCENE_OP_SET_INPUT_DISABLE_MASK = 0x0A1,
    SCENE_OP_SET_INPUT_AUX_MASK = 0x0A2,
    SCENE_OP_SET_REJECTED_INPUT_MASK = 0x0A3,
    SCENE_OP_CLEAR_REJECTED_INPUT_MASK = 0x0A4,
    SCENE_OP_START_INLINE_OBJECT_SCRIPT = 0x0A5,
    SCENE_OP_START_OBJECT_SCRIPT_AND_YIELD = 0x0A6,
    SCENE_OP_WAIT_OBJECT_SCRIPT_AND_SKIP = 0x0A7,
    SCENE_OP_START_OBJECT_SCRIPT = 0x0A8,
    SCENE_OP_WAIT_OBJECT_SCRIPT = 0x0A9,
    SCENE_OP_WAIT_OBJECT_SCRIPTS_BY_OWNER = 0x0AA,
    SCENE_OP_STOP_OBJECT_SCRIPT = 0x0AB,
    SCENE_OP_STOP_OBJECT_SCRIPTS_BY_OWNER = 0x0AC,
    SCENE_OP_PAUSE_OBJECT_SCRIPT = 0x0AD,
    SCENE_OP_PAUSE_OBJECT_SCRIPTS_BY_OWNER = 0x0AE,
    SCENE_OP_RESUME_OBJECT_SCRIPT = 0x0AF,
    SCENE_OP_RESUME_OBJECT_SCRIPTS_BY_OWNER = 0x0B0,
    SCENE_OP_BRANCH_IF = 0x0B1,
    SCENE_OP_BRANCH_IF_OBJECT_PROPERTY = 0x0B2,
    SCENE_OP_BRANCH_IF_SCRIPT_ACTIVE = 0x0B5,
    SCENE_OP_BRANCH_IF_OBJECT_MOTION = 0x0B6,
    SCENE_OP_START_SOUND_TASK_WITH_HANDLE = 0x0B7,
    SCENE_OP_START_SOUND_TASK = 0x0B8,
    SCENE_OP_STOP_SOUND_TASK = 0x0B9,
    SCENE_OP_STOP_ALL_SOUND_TASKS = 0x0BA,
    SCENE_OP_WAIT_SOUND_TASK = 0x0BB,
    SCENE_OP_RENDER_OBJECT_TEXT_TILES = 0x0C2,
    SCENE_OP_GET_OBJECT_TEXT_READY = 0x0C3,
    SCENE_OP_LOAD_UI_RESOURCE = 0x0C7,
    SCENE_OP_WAIT_UI_RESOURCE = 0x0C8,
    SCENE_OP_CREATE_UI_ELEMENT = 0x0C9,
    SCENE_OP_CREATE_OBJECT_UI_ELEMENT = 0x0CA,
    SCENE_OP_WAIT_UI_ELEMENT_READY = 0x0CB,
    SCENE_OP_WAIT_UI_ELEMENT = 0x0CC,
    SCENE_OP_DESTROY_UI_ELEMENT = 0x0CD
};

extern SceneVmManagerLayout *data_ov007_020a6b90;
extern PauseSceneTask *data_ov007_0208e1e0;
extern u8 data_ov007_020905f0[];

extern SceneScriptState *SceneScript_GetObjectState(u32 object_id);
extern void *SceneObject_GetModelById(u32 object_id);
extern int SceneResource_LoadSlot();
extern int SceneResource_IsReady();
extern void SceneObject_SetResourceId();
extern void SceneObject_SetAnimationById();
extern void SceneObjects_SwapSlots();
extern int func_ov007_020883d0();
extern void func_ov007_02088454();
extern int func_ov007_02088468();
extern void SceneObject_SetProperty();
extern void func_ov007_02089000(
    u16 object_id,
    int enabled,
    int mode,
    s32 x,
    s32 y,
    s32 z,
    int argument_6,
    int argument_7,
    int argument_8,
    int argument_9
);
extern void func_ov007_020883a0();
extern void func_ov007_02085ab0();
extern void func_ov007_020881d8();
extern void func_ov007_02087fdc();
extern void func_ov007_02087ebc();
extern void func_ov007_02087d6c();
extern void func_ov007_02087c4c();
extern int SceneObject_IsMotionActiveById();
/* The shipped path command targets this interior overlay-7 entry. */
extern void func_ov007_020724b0(
    void *motion, const void *path, u16 point_count,
    u32 end_time, s32 movement_scale
);
extern void func_ov007_0207e770();
extern void func_ov007_02077110();
extern void func_ov007_02087ba8();
extern void SceneText_RenderObjectTiles(
    SceneObject *object, int mode, int table, int entry, int unused
);
extern int SceneScript_IsReady();
extern void SceneText_LoadArchive();
extern int SceneText_IsLoadComplete();
extern int SceneWindow_Open();
extern int func_ov007_02089980();
extern int func_ov007_020895b8();
extern int SceneWindow_IsOpen();
extern void SceneWindow_Close();
extern s32 FX_Sqrt(s32 value);
extern s32 _s32_div_f(s32 numerator, s32 denominator);

#define ARG(index) command->arguments[(index)]
#define ARG_U16(index) ((u16)ARG(index))
#define ARG_S16(index) ((s16)ARG(index))

static inline s32 SceneVm_PackArgumentPair(
    const ScriptVmCommand *command, int low_index
) {
    return (s32)(((u32)command->arguments[low_index] & 0xFFFF)
        | ((u32)command->arguments[low_index + 1] << 16));
}

static inline int SceneVm_RewindAndYield(
    ScriptVm *vm, ScriptVmState *state, u16 opcode
) {
    const u16 *script = state->script;
    s32 descriptor = (u16)vm->command_descriptors[opcode];
    int mode_words = (descriptor & SCRIPT_VM_HAS_ARGUMENT_MODES) >> 6;
    int result_words = (descriptor & SCRIPT_VM_HAS_RESULT) >> 5;
    int argument_words = descriptor & SCRIPT_VM_ARGUMENT_COUNT_MASK;

    state->script = script - (result_words + argument_words + mode_words + 1);
    return SCRIPT_VM_YIELDED;
}

/* Keep the dynamic opcode load inside this helper for the original schedule. */
static inline int SceneVm_RewindCommandAndYield(
    ScriptVm *vm, ScriptVmState *state, const ScriptVmCommand *command
) {
    const u16 *script = state->script;
    s32 descriptor = (u16)vm->command_descriptors[command->opcode];
    int mode_words = (descriptor & SCRIPT_VM_HAS_ARGUMENT_MODES) >> 6;
    int result_words = (descriptor & SCRIPT_VM_HAS_RESULT) >> 5;
    int argument_words = descriptor & SCRIPT_VM_ARGUMENT_COUNT_MASK;

    state->script = script - (result_words + argument_words + mode_words + 1);
    return SCRIPT_VM_YIELDED;
}

static inline void SceneVm_WriteResult(
    ScriptVm *vm,
    ScriptVmState *state,
    const ScriptVmCommand *command,
    s32 value
) {
    VM_WriteVariable(command->result_variable, value, vm, state);
}

static inline SceneScriptState *SceneVm_GetObjectScript(u32 object_id) {
    return SceneScript_GetObjectState(object_id);
}

static inline SceneScriptState *SceneVm_GetObjectScriptPool(void) {
    return data_ov007_020a6b90->object_scripts;
}

int SceneVm_DispatchCommand(
    ScriptVm *vm, ScriptVmState *state, ScriptVmCommand *command
) {
    SceneObject *object;
    SceneObject *other;
    SceneScriptState *script;
    void *resource;
    s32 value;
    int i;

    switch (command->opcode) {
    case 0x033: return SCRIPT_VM_CONTINUE;
    case 0x034: return SCRIPT_VM_CONTINUE;

    case SCENE_OP_LOAD_OBJECT_RESOURCE: {
        u16 object_id = ARG_U16(0);
        value = SceneVm_PackArgumentPair(command, 1);
        if (!SceneResource_LoadSlot(object_id, value)) {
            return SCRIPT_VM_CONTINUE;
        }
        return SceneVm_RewindAndYield(vm, state, SCENE_OP_LOAD_OBJECT_RESOURCE);
    }

    case 0x036: return SCRIPT_VM_CONTINUE;
    case 0x037: return SCRIPT_VM_CONTINUE;
    case 0x038: return SCRIPT_VM_CONTINUE;

    case SCENE_OP_WAIT_OBJECT_RESOURCE:
        if (!SceneResource_IsReady(ARG_U16(0))) {
            return SCRIPT_VM_CONTINUE;
        }
        return SceneVm_RewindAndYield(vm, state, SCENE_OP_WAIT_OBJECT_RESOURCE);

    case SCENE_OP_BIND_OBJECT_RESOURCE:
        SceneObject_SetResourceId(ARG_U16(0), ARG_U16(1));
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_ACTIVATE_OBJECT_RESOURCE:
        resource = SceneObject_GetModelById(ARG_U16(0));
        if (resource != 0) {
            *(u32 *)((u8 *)resource + 0x7C) &= ~0x200;
        }
        SceneObject_SetAnimationById(ARG_U16(0), ARG(1), ARG(2));
        return SCRIPT_VM_CONTINUE;

    case 0x03C: return SCRIPT_VM_CONTINUE;
    case 0x03D: return SCRIPT_VM_CONTINUE;

    case SCENE_OP_YIELD_03E:
        return SCRIPT_VM_YIELDED;

    case SCENE_OP_SWAP_OBJECT_SLOTS:
        SceneObjects_SwapSlots(ARG_U16(0), ARG_U16(1));
        return SCRIPT_VM_CONTINUE;

    case 0x040: return SCRIPT_VM_CONTINUE;
    case 0x041: return SCRIPT_VM_CONTINUE;
    case SCENE_OP_YIELD_042: return SCRIPT_VM_YIELDED;
    case 0x043: return SCRIPT_VM_CONTINUE;
    case 0x044: return SCRIPT_VM_CONTINUE;
    case SCENE_OP_YIELD_045: return SCRIPT_VM_YIELDED;

    case SCENE_OP_GET_GLOBAL_PROPERTY:
        SceneVm_WriteResult(
            vm, state, command, func_ov007_020883d0(ARG_U16(0))
        );
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_SET_GLOBAL_PROPERTY:
        func_ov007_02088454(ARG_U16(0), ARG(1));
        return SCRIPT_VM_CONTINUE;

    case 0x048: return SCRIPT_VM_CONTINUE;
    case 0x049: return SCRIPT_VM_CONTINUE;
    case 0x04A: return SCRIPT_VM_CONTINUE;

    case SCENE_OP_GET_OBJECT_PROPERTY:
        SceneVm_WriteResult(
            vm,
            state,
            command,
            func_ov007_02088468(ARG_U16(0), ARG_U16(1))
        );
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_SET_OBJECT_PROPERTY_FIXED:
        if ((command->argument_modes & (1 << 2)) == 0) {
            ARG(2) = (ARG(2) & 0xFFFF) | ((u32)ARG(3) << 16);
        }
        /* fallthrough */
    case SCENE_OP_SET_OBJECT_PROPERTY:
        SceneObject_SetProperty(ARG_U16(0), ARG_U16(1), ARG(2));
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_ALIGN_OBJECTS: {
        object = SceneObject_GetById(ARG_U16(0));
        other = SceneObject_GetById(ARG_U16(1));
        /*
         * height += (s16)(other->render_height + 16 * (192 - other->y))
         *         - (s16)(height + 16 * (192 - object->y)) + ARG(2).
         * Preserve the original register allocation and paired narrowing
         * schedule; MWCC's C output interleaves the two conversions.
         * r0-r3 are scratch here.  The other pointer is dead after its two
         * loads; object and command stay live in callee-saved registers.
         */
        asm {
            ldrsh r1, [other, #6]
            ldrsh r3, [object, #6]
            ldrsh r2, [other, #0xd8]
            rsb r1, r1, #192
            ldrsh r0, [object, #0xd8]
            rsb r3, r3, #192
            add r1, r2, r1, lsl #4
            add r3, r0, r3, lsl #4
            mov r2, r1, lsl #16
            mov r1, r3, lsl #16
            mov r2, r2, asr #16
            ldr r3, [command, #0x10]
            sub r1, r2, r1, asr #16
            add r1, r3, r1
            add r0, r0, r1
            strh r0, [object, #0xd8]
        }

        resource = SceneObject_GetModelById(ARG_U16(0));
        other = SceneObject_GetModelById(ARG_U16(1));
        if (resource != 0 && other != 0) {
            ((SceneResource *)resource)->flags.bits.render_group =
                ((SceneResource *)other)->flags.bits.render_group;
        }
        return SCRIPT_VM_CONTINUE;
    }

    case 0x04F: return SCRIPT_VM_CONTINUE;

    case SCENE_OP_SET_OBJECT_POSITION:
        func_ov007_02089000(
            ARG_U16(0), 1, 0, ARG(1), ARG(2), ARG(3), 0, 0, 0, 0
        );
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_GET_OBJECT_COORDINATE: {
        s16 object_y;
        s16 screen_height;
        u16 property_id;
        s16 object_x;

        object = SceneObject_GetById(ARG_U16(0));
        object_y = (s16)(object->y - object->base_y);
        screen_height = (s16)(
            object->render_height + 16 * (192 - object->y));
        property_id = ARG_U16(1);
        object_x = object->x;
        switch (property_id) {
        case 0:
            SceneVm_WriteResult(vm, state, command, object_x);
            break;
        case 1:
            SceneVm_WriteResult(vm, state, command, object_y);
            break;
        case 2:
            SceneVm_WriteResult(vm, state, command, screen_height);
            break;
        }
        return SCRIPT_VM_CONTINUE;
    }

    case 0x052: return SCRIPT_VM_CONTINUE;
    case 0x053: return SCRIPT_VM_CONTINUE;

    case SCENE_OP_WAIT_OBJECT_PENDING_STATE:
        object = SceneObject_GetById(ARG_U16(0));
        value = *(s16 *)((u8 *)object + 0xD6);
        if (value < 0) {
            *(s16 *)((u8 *)object + 0xD6) = 0;
            return SCRIPT_VM_CONTINUE;
        }
        if (value == 0) {
            *(s16 *)((u8 *)object + 0xD6) = 1;
        }
        return SceneVm_RewindAndYield(
            vm, state, SCENE_OP_WAIT_OBJECT_PENDING_STATE
        );

    case SCENE_OP_SET_OBJECT_ANIMATION:
        func_ov007_020883a0(ARG_U16(0), ARG_U16(1));
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_MOVE_OBJECT:
        object = SceneObject_GetById(ARG_U16(0));
        if (ARG_U16(2) == 0 && ARG(7) != 0) {
            ARG(3) += object->x;
            ARG(4) += object->y;
            ARG(5) += object->base_y;
        }
        switch (ARG_U16(2)) {
        case 0:
            SceneObject_AdjustPosition(
                object,
                ARG(3) - object->x,
                ARG(4) - object->y,
                ARG(5) - object->base_y
            );
            break;
        case 1:
            SceneObject_MoveBy(
                object, ARG_U16(1), ARG(3), ARG(4), ARG(5), ARG(6)
            );
            break;
        case 2:
            SceneObject_MoveTo(
                object, ARG_U16(1), ARG(3), ARG(4), ARG(5), ARG(6)
            );
            break;
        case 5:
            other = SceneObject_GetById(ARG_U16(7));
            SceneObject_MoveToObject(
                object,
                ARG_U16(1),
                ARG(3),
                ARG(4),
                ARG(5),
                ARG(6),
                other
            );
            break;
        }
        return SCRIPT_VM_CONTINUE;

    /* MWCC places these two later opcode bodies beside the shared movement
     * cases; retaining that source order also preserves the original layout. */
    case SCENE_OP_START_OBJECT_PATH: {
        u32 path_address;
        void *motion;

        object = SceneObject_GetById(ARG_U16(0));
        motion = SceneObject_BeginMotionChannel(
            object, ARG_U16(1), 0, SceneObject_UpdatePath
        );
        path_address = (u32)state->script + 2 * ARG(3);
        if ((path_address & 3) != 0) {
            path_address &= ~3;
        }
        if ((command->argument_modes & (1 << 5)) == 0) {
            ARG(5) = (s32)((ARG(5) & 0xFFFF) |
                ((u32)ARG(6) << 16)) / 16;
        }
        func_ov007_020724b0(
            motion,
            (const u32 *)(path_address + sizeof(u32)),
            *(const s32 *)path_address / 2,
            (ARG(4) - 1) << 16,
            ARG(5)
        );
        return SCRIPT_VM_CONTINUE;
    }

    case SCENE_OP_GET_OBJECT_MOTION_PROPERTY:
        object = SceneObject_GetById(ARG_U16(0));
        VM_WriteVariable(command->result_variable,
            (*(s32 *)((u8 *)object + 0x3C + 40 * ARG_U16(1)) >> 16) + 1,
            vm, state);
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_MOVE_OBJECT_COMPLEX:
        object = SceneObject_GetById(ARG_U16(0));
        func_ov007_02085ab0(
            object,
            ARG_U16(1),
            ARG(3),
            ARG(4),
            ARG(5),
            ARG(6),
            ARG(7),
            ARG(8),
            ARG(9)
        );
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_MOVE_OBJECT_AT_SPEED: {
        s32 dx;
        s32 dy;
        s32 dz;

        object = SceneObject_GetById(ARG_U16(0));
        if (ARG(6) < 1) {
            ARG(6) = object->default_motion_speed;
        }
        switch (ARG_U16(2)) {
        case 1:
            dx = ARG(3);
            dy = ARG(4);
            dz = ARG(5);
            value = FX_Sqrt((dx * dx + dy * dy + dz * dz) << 12);
            ARG(6) = _s32_div_f(value, ARG(6));
            SceneObject_MoveBy(
                object, ARG_U16(1), ARG(3), ARG(4), ARG(5), ARG(6)
            );
            break;
        case 2:
            dx = ARG(3) - object->x;
            dy = ARG(4) - object->y;
            dz = ARG(5) - object->base_y;
            value = FX_Sqrt((dx * dx + dy * dy + dz * dz) << 12);
            ARG(6) = _s32_div_f(value, ARG(6));
            SceneObject_MoveTo(
                object, ARG_U16(1), ARG(3), ARG(4), ARG(5), ARG(6)
            );
            break;
        case 5:
            dx = ARG(3) - object->x;
            dy = ARG(4) - object->y;
            dz = ARG(5) - object->base_y;
            other = SceneObject_GetById(ARG(7));
            value = dx + other->x;
            dy += other->y;
            dz += other->base_y;
            value = FX_Sqrt((value * value + dy * dy + dz * dz) << 12);
            ARG(6) = _s32_div_f(value, ARG(6));
            object = SceneObject_GetById(ARG(7));
            SceneObject_MoveToObject(
                other,
                ARG_U16(1),
                ARG(3),
                ARG(4),
                ARG(5),
                ARG(6),
                object
            );
            break;
        }
        return SCRIPT_VM_CONTINUE;
    }

    case SCENE_OP_KINEMATIC_AXIS2_A:
        func_ov007_020881d8(vm, state, command, 0);
        return SCRIPT_VM_CONTINUE;
    case SCENE_OP_KINEMATIC_TARGET_A:
        func_ov007_02087fdc(vm, state, command, 0);
        return SCRIPT_VM_CONTINUE;
    case SCENE_OP_KINEMATIC_AXIS2_A_RESULT:
        func_ov007_020881d8(vm, state, command, 1);
        return SCRIPT_VM_CONTINUE;
    case SCENE_OP_KINEMATIC_TARGET_A_RESULT:
        func_ov007_02087fdc(vm, state, command, 1);
        return SCRIPT_VM_CONTINUE;
    case SCENE_OP_KINEMATIC_AXIS2_B:
        func_ov007_02087ebc(vm, state, command, 0);
        return SCRIPT_VM_CONTINUE;
    case SCENE_OP_KINEMATIC_TARGET_B:
        func_ov007_02087d6c(vm, state, command, 0);
        return SCRIPT_VM_CONTINUE;
    case SCENE_OP_KINEMATIC_AXIS2_C:
        func_ov007_02087c4c(vm, state, command, 0);
        return SCRIPT_VM_CONTINUE;
    case SCENE_OP_KINEMATIC_AXIS2_B_RESULT:
        func_ov007_02087ebc(vm, state, command, 1);
        return SCRIPT_VM_CONTINUE;
    case SCENE_OP_KINEMATIC_TARGET_B_RESULT:
        func_ov007_02087d6c(vm, state, command, 1);
        return SCRIPT_VM_CONTINUE;
    case SCENE_OP_KINEMATIC_AXIS2_C_RESULT:
        func_ov007_02087c4c(vm, state, command, 1);
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_BALLISTIC_TERMINAL_SPEED:
        if ((command->argument_modes & (1 << 7)) == 0) {
            ARG(7) = SceneVm_PackArgumentPair(command, 7) / 16;
        }
        object = SceneObject_GetById(ARG_U16(0));
        SceneObject_StartMotionWithVelocityForDuration(
            object,
            ARG_U16(1),
            ARG(2),
            ARG(3),
            ARG(4),
            ARG(5),
            ARG(6),
            ARG(7)
        );
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_BALLISTIC_ACCELERATION:
        if ((command->argument_modes & (1 << 7)) == 0) {
            ARG(7) = SceneVm_PackArgumentPair(command, 7) / 16;
        }
        object = SceneObject_GetById(ARG_U16(0));
        SceneObject_StartMotionWithAccelerationForDuration(
            object,
            ARG_U16(1),
            ARG(2),
            ARG(3),
            ARG(4),
            ARG(5),
            ARG(6),
            ARG(7)
        );
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_BALLISTIC_ROOT:
        object = SceneObject_GetById(ARG_U16(0));
        SceneObject_StartAcceleratedMotionForDuration(
            object,
            ARG_U16(1),
            ARG(2),
            ARG(3),
            ARG(4),
            ARG(5),
            ARG(6),
            ARG(7),
            ARG_U16(8)
        );
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_PARAMETRIC_MOTION:
        object = SceneObject_GetById(ARG_U16(0));
        SceneObject_StartAxisRotation(
            object,
            ARG_U16(1),
            ARG_S16(2),
            ARG_S16(3),
            ARG_S16(4),
            ARG_S16(5),
            ARG_S16(6),
            ARG_S16(7),
            ARG_S16(8),
            ARG_S16(9)
        );
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_WAIT_OBJECT_MOTION:
        if (!SceneObject_IsMotionActiveById(ARG_U16(0), ARG_U16(1))) {
            return SCRIPT_VM_CONTINUE;
        }
        return SceneVm_RewindAndYield(vm, state, SCENE_OP_WAIT_OBJECT_MOTION);

    case SCENE_OP_CANCEL_OBJECT_MOTION:
        object = SceneObject_GetById(ARG_U16(0));
        SceneObject_StopMotionChannel(object, ARG_U16(1), 0);
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_SET_SCREEN_EFFECT:
        if (ARG_U16(0) == 0) {
            func_ov007_0207e770(1, 0, 1);
        } else {
            func_ov007_0207e770(0, 0, 1);
        }
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_RESET_SCREEN_STATE:
        *(s8 *)(data_ov007_020905f0 + 0x110) = -1;
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_GET_SCENE_STATE:
        SceneVm_WriteResult(
            vm, state, command, SceneVM_GetMenuValue(ARG_U16(0))
        );
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_WAIT_SCENE_READY:
        if (data_ov007_020905f0[0x84] == 0) {
            return SCRIPT_VM_CONTINUE;
        }
        return SceneVm_RewindAndYield(vm, state, SCENE_OP_WAIT_SCENE_READY);

    case SCENE_OP_SET_SCENE_MODE:
        PauseScene_RequestExit(data_ov007_0208e1e0, (u8)ARG_U16(0));
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_SET_SCENE_FLAG:
        func_ov007_02077110(ARG_U16(0), ARG_U16(1));
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_GET_SCENE_RECORD_PROPERTY:
        SceneVm_WriteResult(
            vm,
            state,
            command,
            SceneVM_GetSaveValue(ARG_U16(0), ARG_U16(1))
        );
        return SCRIPT_VM_CONTINUE;

    case 0x074: return SCRIPT_VM_CONTINUE;
    case 0x075: return SCRIPT_VM_CONTINUE;
    case 0x076: return SCRIPT_VM_CONTINUE;
    case 0x077: return SCRIPT_VM_CONTINUE;
    case 0x078: return SCRIPT_VM_CONTINUE;
    case SCENE_OP_YIELD_079: return SCRIPT_VM_YIELDED;
    case 0x07A: return SCRIPT_VM_CONTINUE;
    case 0x07B: return SCRIPT_VM_CONTINUE;
    case 0x07C: return SCRIPT_VM_CONTINUE;
    case 0x07D: return SCRIPT_VM_CONTINUE;
    case 0x07E: return SCRIPT_VM_CONTINUE;
    case 0x07F: return SCRIPT_VM_CONTINUE;
    case 0x080: return SCRIPT_VM_CONTINUE;
    case 0x081: return SCRIPT_VM_CONTINUE;
    case 0x082: return SCRIPT_VM_CONTINUE;
    case 0x083: return SCRIPT_VM_CONTINUE;
    case 0x084: return SCRIPT_VM_CONTINUE;
    case 0x085: return SCRIPT_VM_CONTINUE;
    case 0x086: return SCRIPT_VM_CONTINUE;
    case 0x087: return SCRIPT_VM_CONTINUE;
    case 0x088: return SCRIPT_VM_CONTINUE;
    case 0x089: return SCRIPT_VM_CONTINUE;
    case 0x08A: return SCRIPT_VM_CONTINUE;
    case 0x08B: return SCRIPT_VM_CONTINUE;
    case 0x08C: return SCRIPT_VM_CONTINUE;
    case SCENE_OP_YIELD_08D: return SCRIPT_VM_YIELDED;
    case SCENE_OP_YIELD_08E: return SCRIPT_VM_YIELDED;
    case 0x08F: return SCRIPT_VM_CONTINUE;
    case 0x090: return SCRIPT_VM_CONTINUE;
    case 0x091: return SCRIPT_VM_CONTINUE;
    case 0x092: return SCRIPT_VM_CONTINUE;
    case 0x093: return SCRIPT_VM_CONTINUE;
    case 0x094: return SCRIPT_VM_CONTINUE;
    case 0x095: return SCRIPT_VM_CONTINUE;
    case 0x096: return SCRIPT_VM_CONTINUE;
    case 0x098: return SCRIPT_VM_CONTINUE;
    case SCENE_OP_YIELD_099: return SCRIPT_VM_YIELDED;
    case 0x09A: return SCRIPT_VM_CONTINUE;
    case 0x09B: return SCRIPT_VM_CONTINUE;
    case 0x09C: return SCRIPT_VM_CONTINUE;
    case 0x09D: return SCRIPT_VM_CONTINUE;
    case 0x09E: return SCRIPT_VM_CONTINUE;

    case SCENE_OP_CLEAR_PERSISTENT_INPUT:
        SceneInput_ClearPersistentMask();
        return SCRIPT_VM_CONTINUE;
    case SCENE_OP_INJECT_INPUT_MASK:
        func_ov007_02087ba8(ARG_U16(0), ARG_U16(1));
        return SCRIPT_VM_CONTINUE;
    case SCENE_OP_SET_INPUT_DISABLE_MASK:
        SceneInput_SetDisabledMask(ARG_U16(0));
        return SCRIPT_VM_CONTINUE;
    case SCENE_OP_SET_INPUT_AUX_MASK:
        SceneInput_SetAuxiliaryMask(ARG_U16(0));
        return SCRIPT_VM_CONTINUE;
    case SCENE_OP_SET_REJECTED_INPUT_MASK:
        SceneInput_SetRejectedMask(ARG_U16(0));
        return SCRIPT_VM_CONTINUE;
    case SCENE_OP_CLEAR_REJECTED_INPUT_MASK:
        SceneInput_ClearRejectedMask();
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_START_INLINE_OBJECT_SCRIPT:
        SceneScript_StartObjectScript(
            ARG_U16(0), state->script,
            (const SceneScriptState *)state, ARG_U16(1));
        state->script += ARG(2);
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_START_OBJECT_SCRIPT_AND_YIELD:
        SceneScript_StartObjectScript(
            ARG_U16(0), state->script + ARG(2),
            (const SceneScriptState *)state, ARG_U16(1)
        );
        return SCRIPT_VM_YIELDED;

    case SCENE_OP_WAIT_OBJECT_SCRIPT_AND_SKIP:
        script = SceneVm_GetObjectScript(ARG_U16(0));
        if (script->vm_state.script != 0) {
            return SceneVm_RewindAndYield(
                vm, state, SCENE_OP_WAIT_OBJECT_SCRIPT_AND_SKIP
            );
        }
        state->script += ARG(1);
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_START_OBJECT_SCRIPT:
        SceneScript_StartObjectScript(
            ARG_U16(0), state->script + ARG(2),
            (const SceneScriptState *)state, ARG_U16(1)
        );
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_WAIT_OBJECT_SCRIPT:
        script = SceneVm_GetObjectScript(ARG_U16(0));
        if (script->vm_state.script == 0) {
            return SCRIPT_VM_CONTINUE;
        }
        return SceneVm_RewindAndYield(vm, state, SCENE_OP_WAIT_OBJECT_SCRIPT);

    case SCENE_OP_WAIT_OBJECT_SCRIPTS_BY_OWNER: {
        SceneScriptState *entry;
        s16 remaining;

        if (ARG(0) < 1) {
            ARG(0) = ((SceneScriptState *)state)->owner;
        }
        entry = SceneVm_GetObjectScriptPool();
        if (ARG(0) < SCENE_SCRIPT_OWNER_OBJECT) {
            remaining = 40;
            do {
                if (ARG(0) == entry->parent_owner &&
                    entry->vm_state.script != 0) {
                    return SceneVm_RewindAndYield(
                        vm, state, SCENE_OP_WAIT_OBJECT_SCRIPTS_BY_OWNER
                    );
                }
                remaining = (s16)(remaining - 1);
                entry++;
            } while (remaining != 0);
        } else {
            s16 object_remaining = 40;
            u32 object_id = ARG(0) & SCENE_SCRIPT_OWNER_ID_MASK;

            do {
                if (object_id == entry->parent_object_id &&
                    entry->vm_state.script != 0) {
                    return SceneVm_RewindAndYield(
                        vm, state, SCENE_OP_WAIT_OBJECT_SCRIPTS_BY_OWNER
                    );
                }
                object_remaining = (s16)(object_remaining - 1);
                entry++;
            } while (object_remaining != 0);
        }
        return SCRIPT_VM_CONTINUE;
    }

    case SCENE_OP_STOP_OBJECT_SCRIPT:
        SceneVm_GetObjectScript(ARG_U16(0))->vm_state.script = 0;
        return SCRIPT_VM_FINISHED;

    case SCENE_OP_STOP_OBJECT_SCRIPTS_BY_OWNER: {
        SceneScriptState *entry;
        s32 owner;
        s16 remaining;

        if (ARG(0) < 1) {
            ARG(0) = ((SceneScriptState *)state)->owner;
        }
        owner = ARG(0);
        entry = SceneVm_GetObjectScriptPool();
        if (owner < SCENE_SCRIPT_OWNER_OBJECT) {
            remaining = 40;
            do {
                owner = ARG(0);
                if (owner == entry->parent_owner &&
                    entry->vm_state.script != 0) {
                    SceneVm_GetObjectScript(owner)->vm_state.script = 0;
                    SceneVm_GetObjectScript(ARG(0))->vm_state.stack_depth = 0;
                }
                remaining = (s16)(remaining - 1);
                entry++;
            } while (remaining > 0);
        } else {
            s16 object_remaining = 40;
            u32 object_id = owner & SCENE_SCRIPT_OWNER_ID_MASK;

            do {
                if (object_id == entry->parent_object_id &&
                    entry->vm_state.script != 0) {
                    SceneVm_GetObjectScript(owner)->vm_state.script = 0;
                    SceneVm_GetObjectScript(ARG(0))->vm_state.stack_depth = 0;
                    break;
                }
                object_remaining = (s16)(object_remaining - 1);
                entry++;
            } while (object_remaining != 0);
        }
        return SCRIPT_VM_CONTINUE;
    }

    case SCENE_OP_PAUSE_OBJECT_SCRIPT:
        SceneVm_GetObjectScript(ARG_U16(0))->flags.bits.paused = 1;
        return SCRIPT_VM_YIELDED;

    case SCENE_OP_PAUSE_OBJECT_SCRIPTS_BY_OWNER: {
        SceneScriptState *entry;
        s32 owner;
        s16 remaining;

        if (ARG(0) < 1) {
            ARG(0) = ((SceneScriptState *)state)->owner;
        }
        owner = ARG(0);
        entry = SceneVm_GetObjectScriptPool();
        if (owner < SCENE_SCRIPT_OWNER_OBJECT) {
            remaining = 40;
            do {
                if (ARG(0) == entry->parent_owner &&
                    entry->vm_state.script != 0) {
                    entry->flags.bits.paused = 1;
                }
                remaining = (s16)(remaining - 1);
                entry++;
            } while (remaining > 0);
        } else {
            u32 object_id = owner & SCENE_SCRIPT_OWNER_ID_MASK;

            remaining = 40;
            do {
                if (object_id == entry->parent_object_id &&
                    entry->vm_state.script != 0) {
                    entry->flags.bits.paused = 1;
                    break;
                }
                remaining = (s16)(remaining - 1);
                entry++;
            } while (remaining != 0);
        }
        return SCRIPT_VM_CONTINUE;
    }

    case SCENE_OP_RESUME_OBJECT_SCRIPT:
        SceneVm_GetObjectScript(ARG_U16(0))->flags.bits.paused = 0;
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_RESUME_OBJECT_SCRIPTS_BY_OWNER: {
        SceneScriptState *entry;
        s32 owner;
        s16 remaining;

        if (ARG(0) < 1) {
            ARG(0) = ((SceneScriptState *)state)->owner;
        }
        owner = ARG(0);
        entry = SceneVm_GetObjectScriptPool();
        if (owner < SCENE_SCRIPT_OWNER_OBJECT) {
            remaining = 40;
            do {
                if (ARG(0) == entry->parent_owner &&
                    entry->vm_state.script != 0) {
                    entry->flags.bits.paused = 0;
                }
                remaining = (s16)(remaining - 1);
                entry++;
            } while (remaining > 0);
        } else {
            u32 object_id = owner & SCENE_SCRIPT_OWNER_ID_MASK;

            remaining = 40;
            do {
                if (object_id == entry->parent_object_id &&
                    entry->vm_state.script != 0) {
                    entry->flags.bits.paused = 0;
                    break;
                }
                remaining = (s16)(remaining - 1);
                entry++;
            } while (remaining != 0);
        }
        return SCRIPT_VM_CONTINUE;
    }

    case SCENE_OP_BRANCH_IF:
        value = 0;
        switch (ARG_U16(0)) {
        case SCRIPT_VM_EQUAL:
            value = ARG(1) == ARG(2);
            break;
        case SCRIPT_VM_NOT_EQUAL:
            value = ARG(1) != ARG(2);
            break;
        case SCRIPT_VM_LESS_THAN:
            value = ARG(1) < ARG(2);
            break;
        case SCRIPT_VM_GREATER_THAN:
            value = ARG(1) > ARG(2);
            break;
        case SCRIPT_VM_LESS_OR_EQUAL:
            value = ARG(1) <= ARG(2);
            break;
        case SCRIPT_VM_GREATER_OR_EQUAL:
            value = ARG(1) >= ARG(2);
            break;
        case SCRIPT_VM_BITS_SET:
            value = ARG(1) & ARG(2);
            break;
        case SCRIPT_VM_EITHER_NONZERO:
            value = ARG(1) | ARG(2);
            break;
        case SCRIPT_VM_VALUES_DIFFER:
            value = ARG(1) ^ ARG(2);
            break;
        case SCRIPT_VM_ZERO:
            value = ARG(1) == 0;
            break;
        case SCRIPT_VM_NOT_NEGATIVE_ONE:
            value = ~ARG(1);
            break;
        }
        if (ARG_U16(4) == (value == ARG(3))) {
            state->script += ARG(5);
        }
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_BRANCH_IF_OBJECT_PROPERTY:
        value = func_ov007_02088468(ARG_U16(0), ARG(1)) > 0;
        if (ARG_U16(2) == value) {
            state->script += ARG(3);
        }
        return SCRIPT_VM_CONTINUE;

    case 0x0B3: return SCRIPT_VM_CONTINUE;
    case 0x0B4: return SCRIPT_VM_CONTINUE;

    case SCENE_OP_BRANCH_IF_SCRIPT_ACTIVE:
        script = SceneScript_GetState(ARG_U16(0));
        if (ARG_U16(1) == (script->vm_state.script != 0)) {
            state->script += ARG(2);
        }
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_BRANCH_IF_OBJECT_MOTION:
        value = SceneObject_IsMotionActiveById(ARG_U16(0), ARG_U16(1));
        if (ARG_U16(2) == value) {
            state->script += ARG(3);
        }
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_START_SOUND_TASK_WITH_HANDLE:
        value = SceneSound_PlayRepeated(
            ARG_U16(0), ARG_S16(1), ARG_S16(2), 0
        );
        if (value != -1) {
            SceneVm_WriteResult(vm, state, command, value);
        }
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_START_SOUND_TASK:
        SceneSound_PlayRepeated(ARG_U16(0), ARG_S16(1), 0, 0);
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_STOP_SOUND_TASK:
        if (ARG(0) != -1) {
            SceneSound_StopTask(ARG_U16(0));
        }
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_STOP_ALL_SOUND_TASKS:
        i = 0;
        do {
            if (data_ov007_020a6b90->sound_tasks[i] != 0) {
                SceneSound_StopTask(i);
            }
            i++;
        } while (i < 16);
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_WAIT_SOUND_TASK:
        if (ARG(0) == -1) {
            return SCRIPT_VM_CONTINUE;
        }
        if (data_ov007_020a6b90->sound_tasks[ARG_U16(0)] == 0) {
            return SCRIPT_VM_CONTINUE;
        }
        return SceneVm_RewindAndYield(vm, state, SCENE_OP_WAIT_SOUND_TASK);

    case 0x0BC: return SCRIPT_VM_CONTINUE;
    case 0x0BE: return SCRIPT_VM_CONTINUE;
    case 0x0BF: return SCRIPT_VM_CONTINUE;
    case 0x0BD: return SCRIPT_VM_CONTINUE;
    case 0x0C0: return SCRIPT_VM_CONTINUE;
    case SCENE_OP_RENDER_OBJECT_TEXT_TILES:
        object = SceneObject_GetById(ARG(0));
        SceneText_RenderObjectTiles(object, ARG(1), ARG(2), ARG(3), ARG(4));
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_GET_OBJECT_TEXT_READY:
        object = SceneObject_GetById(ARG(0));
        SceneVm_WriteResult(
            vm,
            state,
            command,
            SceneScript_IsReady(object, ARG(1), ARG(2))
        );
        return SCRIPT_VM_CONTINUE;

    case 0x0C1: return SCRIPT_VM_CONTINUE;
    case 0x0C4: return SCRIPT_VM_CONTINUE;
    case 0x0C5: return SCRIPT_VM_CONTINUE;
    case 0x0C6: return SCRIPT_VM_CONTINUE;

    case SCENE_OP_LOAD_UI_RESOURCE:
        SceneText_LoadArchive(ARG_U16(0));
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_WAIT_UI_RESOURCE:
        if (SceneText_IsLoadComplete()) {
            return SCRIPT_VM_CONTINUE;
        }
        return SceneVm_RewindCommandAndYield(vm, state, command);

    case SCENE_OP_CREATE_UI_ELEMENT:
        value = VM_ReadVariable(SCRIPT_VM_VAR_SAVE_WORDS_40, vm, state);
        value = SceneWindow_Open(
            ARG(1),
            ARG(0),
            ARG(2),
            ARG(3),
            ARG(4),
            ARG(5),
            ARG(6),
            ARG(7),
            ARG(8),
            ARG(9),
            ARG(10),
            ARG(11),
            ARG(12),
            ARG(13),
            ARG(14),
            value,
            gSaveData + 0x44
        );
        SceneVm_WriteResult(vm, state, command, value);
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_CREATE_OBJECT_UI_ELEMENT:
        object = SceneObject_GetById(ARG(1));
        value = VM_ReadVariable(SCRIPT_VM_VAR_SAVE_WORDS_40, vm, state);
        value = func_ov007_02089980(
            ARG(0),
            object,
            ARG(2),
            ARG(3),
            ARG(4),
            ARG(5),
            ARG(6),
            ARG(7),
            ARG(8),
            ARG(9),
            ARG(10),
            ARG(11),
            value,
            gSaveData + 0x44
        );
        SceneVm_WriteResult(vm, state, command, value);
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_WAIT_UI_ELEMENT_READY:
        if (func_ov007_020895b8(ARG(0)) &&
            SceneWindow_IsOpen(ARG(0))) {
            return SceneVm_RewindCommandAndYield(vm, state, command);
        }
        return SCRIPT_VM_CONTINUE;

    case SCENE_OP_WAIT_UI_ELEMENT:
        if (!func_ov007_020895b8(ARG(0))) {
            return SCRIPT_VM_CONTINUE;
        }
        return SceneVm_RewindCommandAndYield(vm, state, command);

    case SCENE_OP_DESTROY_UI_ELEMENT:
        SceneWindow_Close(ARG(0));
        return SCRIPT_VM_CONTINUE;

    case 0x0CE: return SCRIPT_VM_CONTINUE;
    case 0x0CF: return SCRIPT_VM_CONTINUE;
    case 0x0D0: return SCRIPT_VM_CONTINUE;
    case 0x0D1: return SCRIPT_VM_CONTINUE;

    default:
        return SCRIPT_VM_CONTINUE;

    /* The ROM retains a distinct legacy slot after the default epilogue. */
    case 0x097:
        break;
    }

    return SCRIPT_VM_CONTINUE;
}

#undef ARG
#undef ARG_U16
#undef ARG_S16
