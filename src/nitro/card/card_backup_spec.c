/* Cartridge backup geometry and operation delays.
 * The type packs the device kind in bits 0..7 and log2(capacity) in bits 8..15.
 * Supported type constants have a capacity exponent below 32. The protocol
 * record retains unrelated request fields while its geometry is refreshed.
 */
#include <nitro/card.h>

void CARDi_SetBackupSpec(int type)
{
    CardCommand *command = cardi_common.command;
    u32 size;
    MI_CpuFill8(&command->total_size, 0, 40);
    command->backup_type = type;
    if (!type) return;
    size = 1u << ((type >> 8) & 255);
    command->total_size = size;
    if ((type & 255) == 1) {
        switch (size) {
        case 0x200:
            command->page_size = 16;
            command->address_width = 1;
            command->write_delay = 79;
            return;
        case 0x2000:
            command->page_size = 32;
            command->address_width = 2;
            command->write_delay = 79;
            return;
        case 0x10000:
            command->page_size = 128;
            command->address_width = 2;
            command->write_delay = 158;
            return;
        default: goto invalid;
        }
    } else if ((type & 255) == 2) {
        switch (size) {
        case 0x40000:
            command->erase_delay = 395;
            command->erase_timeout = 4735;
            command->program_timeout = 4735;
            command->total_erase_timeout = 78901;
            command->total_erase_delay = 78901;
            break;
        case 0x80000:
        case 0x100000:
            command->erase_delay = 395;
            command->erase_timeout = 0;
            command->program_timeout = 4735;
            command->total_erase_timeout = 78901;
            break;
        default: goto invalid;
        }
        command->sector_size = 0x10000;
        command->page_size = 256;
        command->address_width = 3;
        command->write_delay = 79;
        return;
    } else if ((type & 255) == 3) {
        switch (size) {
        case 0x2000:
        case 0x8000:
            command->page_size = size;
            command->address_width = 2;
            return;
        }
    }
invalid:
    /* The native failure path clears the cached record before reloading the
     * shared command and reporting an unsupported device. Keep that short
     * store/reload sequence together; MWCC otherwise interleaves its loads.
     */
    asm {
        mov r1, #0
        str r1, [command, #4]
        str r1, [command, #0x18]
        ldr r0, =cardi_common
        mov r1, #3
        ldr r0, [r0]
        str r1, [r0]
    }
}
