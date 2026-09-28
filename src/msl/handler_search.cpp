/* Search encoded catch actions using a private copy of the unwind cursor.
 * An exception-specification failure transfers to its own landing pad and
 * preserves the action pointer for the later unexpected-exception handling.
 */
#include "exception_search_internal.h"

/* MWCC emits the function sections in reverse definition order. */

extern "C" {
void func_020477b4(MslSearchContext *,MslSearchRecord *,const u8 *);
void func_02046de0(MslSearchContext *,MslSearchRecord *,u32);
/* The specification failure record also keeps the matching action pointer. */
struct MslSpecificationCatch {
    void *object;
    const u8 *type;
    void *destructor;
    u32 unknown12, unknown16;
    const u8 *handler;
};
typedef char MslSpecificationCatchSize[sizeof(MslSpecificationCatch) == 24 ? 1 : -1];
void MSL_DispatchExceptionSpecification(MslSearchContext *context,MslSearchRecord *record,const MslExceptionSpec *specification,const u8 *handler)
{
    func_020477b4(context,record,handler);
    MslSpecificationCatch *caught=(MslSpecificationCatch *)(context->frame_pointer+specification->frame_offset);
    caught->object=context->object;
    caught->type=context->type;
    caught->destructor=context->destructor;
    caught->handler=handler;
    func_02046de0(context,record,record->lookup.function+specification->landing_offset);
}
}

extern "C" {
int func_0204803c(MslSearchCursor *);
void MSL_Terminate(void);
const u8 *MSL_DecodeSigned(const u8 *,s32 *);

const u8 *MSL_DecodeUnsigned(const u8 *,u32 *);
int func_02048614(const u8 *,const u8 *,s32 *);
int MSL_MatchesExceptionSpecification(const u8 *,const MslExceptionSpec *);
void MSL_DispatchExceptionSpecification(MslSearchContext *,MslSearchRecord *,const MslExceptionSpec *,const u8 *);

const u8 *MSL_FindCatchHandler(MslSearchContext *context,MslSearchRecord *record,s32 *adjustment)
{
    struct {const u8 *type; u32 landing_offset; s32 frame_offset;} caught;
    MslExceptionSpec specification;
    MslSearchCursor cursor;
    cursor.record.lookup.function=record->lookup.function;
    cursor.record.lookup.instructions=record->lookup.instructions;
    cursor.record.lookup.handler=record->lookup.handler;
    cursor.record.lookup.table_start=record->lookup.table_start;
    cursor.record.lookup.table_end=record->lookup.table_end;
    cursor.record.unknown20=record->unknown20;
    cursor.context.type=context->type;
    cursor.context.object=context->object;
    cursor.context.destructor=context->destructor;
    cursor.context.active_catch=context->active_catch;
    cursor.context.address=context->address;
    cursor.context.stack_pointer=context->stack_pointer;
    cursor.context.frame_pointer=context->frame_pointer;
    cursor.context.state=context->state;
    int kind=MSL_GetUnwindHandlerKind(&cursor.record.lookup);
    for (;;) {
        switch (kind) {
        case 0: case 2: case 3: case 4: case 5: case 6: case 7: case 8:
        case 9: case 10: case 11: case 13: case 16: case 17: case 18: case 19:
            break;
        case 12: {
            const u8 *input=cursor.record.lookup.handler;
            caught.type=(const u8 *)((u32)input[1] | (u32)input[2]<<8 | (u32)input[3]<<16 | (u32)input[4]<<24);
            input=MSL_DecodeUnsigned(input+5,&caught.landing_offset);
            MSL_DecodeSigned(input,&caught.frame_offset);
            if (!func_02048614(context->type,caught.type,adjustment)) break;
            goto found;
        }
        case 15: {
            const u8 *input=MSL_DecodeUnsigned(cursor.record.lookup.handler+1,&specification.count);
            input=MSL_DecodeUnsigned(input,&specification.landing_offset);
            specification.types=MSL_DecodeSigned(input,&specification.frame_offset);
            if (!MSL_MatchesExceptionSpecification(context->type,&specification))
                MSL_DispatchExceptionSpecification(context,record,&specification,cursor.record.lookup.handler);
            break;
        }
        default:
            MSL_Terminate();
            goto found;
        }
        kind=func_0204803c(&cursor);
    }
found:
    return cursor.record.lookup.handler;
}
}
