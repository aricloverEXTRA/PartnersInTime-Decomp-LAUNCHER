#ifndef PIT_MSL_EXCEPTION_SEARCH_INTERNAL_H
#define PIT_MSL_EXCEPTION_SEARCH_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif
#include "unwind_internal.h"

/* Search cursors copy the exception prefix separately from the saved state. */
typedef struct MslSearchSavedState {u32 words[21];} MslSearchSavedState;
typedef struct MslSearchContext {
    const u8 *type;
    void *object;
    void *destructor;
    void *active_catch;
    u32 address, stack_pointer;
    u8 *frame_pointer;
    MslSearchSavedState state;
} MslSearchContext;
typedef struct MslSearchRecord {
    MslUnwindRecord lookup;
    u32 unknown20;
} MslSearchRecord;
typedef struct MslSearchCursor {
    MslSearchRecord record;
    MslSearchContext context;
} MslSearchCursor;
/* Only the active catch's object/type prefix is read by the search. */
typedef struct MslActiveCatch {void *object; const u8 *type;} MslActiveCatch;
typedef struct MslExceptionSpec {
    u32 count;
    u32 landing_offset;
    s32 frame_offset;
    const u8 *types;
} MslExceptionSpec;

typedef char MslSearchContextSize[sizeof(MslSearchContext) == 112 ? 1 : -1];
typedef char MslSearchRecordSize[sizeof(MslSearchRecord) == 24 ? 1 : -1];
typedef char MslSearchCursorSize[sizeof(MslSearchCursor) == 136 ? 1 : -1];
typedef char MslSearchSavedOffset[(u32)&((MslSearchContext *)0)->state == 28 ? 1 : -1];
typedef char MslExceptionSpecSize[sizeof(MslExceptionSpec) == 16 ? 1 : -1];
#ifdef __cplusplus
}
#endif
#endif
