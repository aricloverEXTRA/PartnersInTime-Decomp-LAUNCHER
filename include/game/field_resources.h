#ifndef PIT_GAME_FIELD_RESOURCES_H
#define PIT_GAME_FIELD_RESOURCES_H

/* Room resource records, loaded buffers and the temporary archive requests. */
#include <game/field_archive.h>
#include <game/field_spawn.h>
#include <game/field_entity.h>
#include <game/model_resource.h>

typedef struct FieldPrimaryResourceRecord {
    u16 animation_entry, unknown_02, bounds_entry;
    u8 unknown_06;
    union {
        u8 flags;
        struct {
            u8 unknown_00 : 1, direction_mode : 2, unknown_03_07 : 5;
        } flag_bits;
    };
} FieldPrimaryResourceRecord;

typedef struct FieldSecondaryResourceRecord {
    u16 data_entry, auxiliary_entry;
    u8 extent, unknown_05[3];
} FieldSecondaryResourceRecord;

typedef struct FieldPrimaryResource {
    u32 id;
    FieldPrimaryResourceRecord *record;
    void *graphics;
    void *animation;
    void *bounds;
    union {
        u32 flags;
        struct {
            u32 unknown_00 : 1, alternate_heap : 1, graphics_size : 30;
        } bits;
    };
} FieldPrimaryResource;

typedef struct FieldSecondaryResource {
    u32 id;
    union {
        u16 flags;
        struct {
            u16 alternate : 1, extent : 15;
        } bits;
    };
    u16 reserved;
    void *data;
    void *auxiliary;
    FieldSecondaryResourceRecord *record;
} FieldSecondaryResource;

/* Room palettes retain the native allocator result after the 20-byte palette. */
typedef struct FieldPaletteResource {
    GameSpritePalette palette;
    u8 allocation_result;
    u8 unknown_15[3];
} FieldPaletteResource;

typedef char FieldPaletteResource_SizeCheck[sizeof(FieldPaletteResource) == 24 ? 1 : -1];

typedef ModelResourceDescriptor FieldResourceDescriptor;

/* Partial view of the field context used while loading room resources. */
typedef struct FieldResourceContext {
    u8 unknown_0000[0x24];
    u16 read_count;
    u16 compressed_count;
    u8 unknown_0028[0x8C];
    FieldPrimaryResource *primary[2];
    FieldSecondaryResource *secondary[2];
    FieldSecondaryResource *alternate[2];
    union {
        u8 unknown_00cc[0x2210];
        struct {
            FieldPaletteResource *palettes[2], *alternate_palettes[2];
            u8 unknown_00dc[0x2200];
        };
    };
    u8 **animation_buffers[2];
    u8 *special_animations[4];
    u8 unknown_22f4[0xC];
    const u32 *primary_ids[2];
    const u32 *secondary_ids[2];
    const u32 *alternate_ids[2];
    const FieldSpawnRecord *spawn_records[2];
    u8 unknown_2320[0x10];
    const u32 *animation_ids[2];
    int primary_counts[2];
    int secondary_counts[2];
    int alternate_counts[2];
    int spawn_counts[2];
    int animation_counts[2];
    void *room_extra;
    union {
        u8 unknown_2364[0x78];
        struct {
            void *extra_room_buffer;
            u8 unknown_2368[4];
            void *room_buffers[20]; /* Cleanup owns slots 1 through 19. */
            u8 unknown_23bc[32];
        };
    };
    FieldArchiveRequest *reads;
    ArchiveCompressedRequest *compressed_reads;
    FieldArchiveRequest *read_cursor;
    ArchiveCompressedRequest *compressed_cursor;
    FieldArchive *archive;
    u8 unknown_23f0[0x4];
    u16 room_id;
    struct {
        u8 main : 4, temporary : 4;
    } heaps;
    u8 unknown_23f7[0x61];
    u32 unknown_2458;
    u8 unknown_245c[0x84];
    u8 *scripts[2];
    u8 unknown_24e8[0x10];
    void *room_resource;
    u8 unknown_24fc[0x4DC];
    FieldRuntimeEntity *entities[36];
    FieldPrimaryResource *shared_resources;
    u8 unknown_2a6c[0xC6];
    u8 entity_count;
    u8 trailing_entity_count;
    s8 special_resources[6];
} FieldResourceContext;

typedef char FieldResourceContext_SizeCheck[sizeof(FieldResourceContext) == 11068 ? 1 : -1];

typedef char FieldResourceContext_ExtraRoomBufferOffsetCheck[
    (u32)&((FieldResourceContext *)0)->extra_room_buffer == 0x2364 ? 1 : -1];
typedef char FieldResourceContext_RoomBuffersOffsetCheck[
    (u32)&((FieldResourceContext *)0)->room_buffers == 0x236c ? 1 : -1];

typedef char FieldPrimaryResourceRecord_SizeCheck[sizeof(FieldPrimaryResourceRecord) == 8 ? 1 : -1];
typedef char FieldSecondaryResourceRecord_SizeCheck[sizeof(FieldSecondaryResourceRecord) == 8 ? 1 : -1];

typedef char FieldPrimaryResource_SizeCheck[sizeof(FieldPrimaryResource) == 24 ? 1 : -1];
typedef char FieldSecondaryResource_SizeCheck[sizeof(FieldSecondaryResource) == 20 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
void FieldEntity3D_ConfigureAuxiliaryResources(FieldRuntimeEntity *entity,
    const FieldPrimaryResource *primary, const FieldSecondaryResource *secondary,
    FieldPaletteResource *palette);
void FieldEntity_RebindRendererResources(FieldRuntimeEntity *entity, const FieldPrimaryResource *primary,
                                         const FieldSecondaryResource *secondary,
                                         FieldPaletteResource *palette, int animation, u8 update_bounds,
                                         int speed);
void FieldEntity_ConfigureRendererResources(FieldRuntimeEntity *entity, const FieldPrimaryResource *primary,
                                            const FieldSecondaryResource *secondary,
                                            FieldPaletteResource *palette, int speed);
int FieldResources_UsesAlternateHeap(void *field_context, int set, int resource_index);
FieldPrimaryResource *FieldResources_FindShared(void *field_context, u32 id);
void FieldResources_AllocateRoomBuffers(FieldResourceContext *field);
void FieldEntity_SetPaletteProfile(FieldResourceContext *field, FieldRuntimeEntity *entity, int selector);
void FieldResources_LoadRoomScripts(void *field_context);
#ifdef __cplusplus
}
#endif

#endif
