/* Cache an archive's allocation and filename tables in caller-owned RAM.
 * Return the required capacity even when the supplied buffer is too small.
 * Keep the original buffer for ownership; only the table pointers are aligned.
 */
#include <nitro/fs.h>

u32 FS_LoadArchiveTables(FsArchive *archive, void *buffer, u32 size) {
    u32 unaligned_size = archive->fat_size + archive->fnt_size + 32;
    u32 required;
    /* Keep the native reserve-then-round additions separate. MWCC folds
     * the C expression to a single +63; this instruction preserves +32,+31. */
    asm { add unaligned_size, unaligned_size, #31 }
    required = unaligned_size & ~31;
    if (required <= size) {
        u8 *destination = (u8 *)(((u32)buffer + 31) & ~31);
        FsFile file;
        u32 table_offset;
        FS_InitFile(&file);
        table_offset = archive->fat;
        if (FS_OpenFileDirect(&file, archive, table_offset, table_offset + archive->fat_size, -1)) {
            if (FS_ReadFile(&file, destination, archive->fat_size) < 0)
                MI_CpuFill8(destination, 0, archive->fat_size);
            FS_CloseFile(&file);
        }
        archive->fat = (u32)destination;
        destination += archive->fat_size;
        table_offset = archive->fnt;
        if (FS_OpenFileDirect(&file, archive, table_offset, table_offset + archive->fnt_size, -1)) {
            if (FS_ReadFile(&file, destination, archive->fnt_size) < 0)
                MI_CpuFill8(destination, 0, archive->fnt_size);
            FS_CloseFile(&file);
        }
        archive->fnt = (u32)destination;
        archive->table_buffer = buffer;
        archive->table_read = FSi_ReadMemoryCore;
        archive->flags |= 4;
    }
    return required;
}
