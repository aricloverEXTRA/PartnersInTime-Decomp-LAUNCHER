/* Begin and finish a packed geometry command buffer. */

#include <nitro/gx_command_list.h>

/* Reserve the first word for four packed opcodes. The buffer itself is not
 * cleared: the command writers initialize its contents as they append. */
void GxCommandList_Begin(GxCommandList *list, void *buffer, u32 capacity)
{
    list->capacity = capacity;
    list->buffer = buffer;
    list->command = buffer;
    list->parameters = (u32 *)buffer + 1;
    list->padding_required = 0;
}

/* Complete the final opcode group and any required parameter padding. */
u32 GxCommandList_End(GxCommandList *list) {
    if (list->buffer == list->command) return 0;
    switch ((u32)list->command & 3) {
    case 0:
        return list->command - list->buffer;
    case 1:
        *list->command++ = 0;
        /* fall through */
    case 2:
        *list->command++ = 0;
        /* fall through */
    case 3:
        *list->command++ = 0;
    }
    if (list->padding_required) {
        *list->parameters++ = 0;
        list->padding_required = 0;
    }
    list->command = (u8 *)list->parameters;
    return list->command - list->buffer;
}
