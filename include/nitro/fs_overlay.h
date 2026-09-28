#ifndef NITRO_FS_OVERLAY_H
#define NITRO_FS_OVERLAY_H

/*
 * Overlay images: the table entry describing where one loads, and the load,
 * start, end and unload that bracket its lifetime.
 */

#include <nitro/fs.h>

/* The linker defines each ID as a symbol value, not a stored integer. */
#define FS_EXTERN_OVERLAY(name) extern u32 SDK_OVERLAY_##name##_ID[1]
#define FS_OVERLAY_ID(name) ((u32)&SDK_OVERLAY_##name##_ID)

typedef struct FsRomTable { u32 offset; u32 size; } FsRomTable;
typedef struct FsOverlayInfo {
    u32 id;
    u8 *address;
    u32 ram_size;
    u32 bss_size;
    void (**initializers)(void);
    void (**initializers_end)(void);
    u32 file_id;
    u32 compressed_size : 24;
    u32 flags : 8;
    int processor;
    u32 file_offset;
    u32 file_size;
} FsOverlayInfo;

typedef char FsOverlayInfoSizeCheck[(sizeof(FsOverlayInfo) == 44) ? 1 : -1];
typedef char FsRomTableSizeCheck[(sizeof(FsRomTable) == 8) ? 1 : -1];

extern FsArchive fsi_arc_rom;
extern FsRomTable fsi_ovt9;
extern FsRomTable fsi_ovt7;

u32 FSi_GetOverlayBinarySize(const FsOverlayInfo *overlay);
void FS_ClearOverlayImage(const FsOverlayInfo *overlay);
FsFileId FS_GetOverlayFileID(const FsOverlayInfo *overlay);
int FSi_LoadOverlayInfoCore(FsOverlayInfo *overlay, int processor, u32 id,
    FsArchive *archive, u32 arm9_offset, u32 arm9_size, u32 arm7_offset, u32 arm7_size);
int FS_LoadOverlayInfo(FsOverlayInfo *overlay, int processor, u32 id);
int FS_LoadOverlayImage(FsOverlayInfo *overlay);
void FS_StartOverlay(FsOverlayInfo *overlay);
void FS_EndOverlay(FsOverlayInfo *overlay);
int FS_UnloadOverlayImage(FsOverlayInfo *overlay);

#endif
