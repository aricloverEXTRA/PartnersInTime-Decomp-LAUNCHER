/* Dispatch a new exception or rethrow to its matching catch handler.
 * The lookup prefix is shared with unwind_internal.h. Catch search also uses
 * an opaque sixth word; this dispatcher adds the handler's encoded type.
 */
#include "unwind_internal.h"

/* The first five context words describe the current exception. */
typedef union MslDispatchContext {
    MslUnwindContext unwind;
    struct {
        const u8 *type;
        void *object;
        void *destructor;
        void *active_catch;
        u32 address;
    } exception;
} MslDispatchContext;
typedef struct MslCatchFrame {
    MslUnwindRecord lookup;
    u32 unknown_14;
    const u8 *catch_type;
} MslCatchFrame;
typedef char MslDispatchContextSize[sizeof(MslDispatchContext) == 112 ? 1 : -1];
typedef char MslCatchFrameSize[sizeof(MslCatchFrame) == 28 ? 1 : -1];

void MSL_Terminate(void);
void *MSL_FindActiveCatch(MslDispatchContext *, const MslCatchFrame *);
const u8 *MSL_FindCatchHandler(MslDispatchContext *, MslCatchFrame *, s32 *);
const u8 *MSL_DecodeSigned(const u8 *, s32 *);
void func_020477b4(MslDispatchContext *, MslCatchFrame *, const u8 *);
void MSL_SetupCatchRecord(MslDispatchContext *, s32, s32);
void func_02046de0(MslDispatchContext *, MslCatchFrame *, u32);

void MSL_DispatchException(MslDispatchContext *context)
{
    s32 adjustment;
    /* Keep the frame description and its decoded catch operands together. */
    struct {
        MslCatchFrame frame;
        u32 landing_offset;
        s32 frame_offset;
    } decoded;
    const u8 *handler, *input;
    MSL_LookupUnwindRecord(context->exception.address, &decoded.frame.lookup);
    if (!decoded.frame.lookup.instructions)
        MSL_Terminate();
    MSL_DecodeFrameHeader(&context->unwind, &decoded.frame.lookup);
    if (context->exception.type)
        context->exception.active_catch = 0;
    else
        context->exception.active_catch = MSL_FindActiveCatch(context, &decoded.frame);
    handler = MSL_FindCatchHandler(context, &decoded.frame, &adjustment);
    decoded.frame.catch_type = (const u8 *)((u32)handler[1] | (u32)handler[2] << 8
        | (u32)handler[3] << 16 | (u32)handler[4] << 24);
    input = MSL_DecodeUnsigned(handler + 5, &decoded.landing_offset);
    MSL_DecodeSigned(input, &decoded.frame_offset);
    func_020477b4(context, &decoded.frame, handler);
    MSL_SetupCatchRecord(context, decoded.frame_offset, adjustment);
    /* Restore the saved ARM registers and stack, then enter the landing pad. */
    func_02046de0(context, &decoded.frame,
                  decoded.frame.lookup.function + decoded.landing_offset);
}
