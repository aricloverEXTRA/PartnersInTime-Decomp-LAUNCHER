/*
 * Render model texture restore (ARM9 resident, 0x0200F804-0x0200FB34).
 *
 * Restore presentation and animation state, allocate texture/palette records,
 * and select or rebuild the texture-offset table. The related wrappers query
 * conversion space, rebind the controller and release both allocations.
 */

extern "C" {
#include <game/graphics_resource.h>
}
#include <game/battle_scene.h>
#include <game/model_resource.h>

extern "C" {
void func_0200a45c(BattleModel *, const void *);
void func_0202cc58(const void *, void *, u32);
void func_0202cd2c(const void *, void *, u32);
int func_02009224(BattleModel *, int);

void BattleRenderModel_RestoreResources(BattleModel *model, const ModelRenderDescriptor *descriptor)
{
    model->screen = descriptor->resource_flags.screen;
    model->resource_pixels = descriptor->graphics;
    func_0200a45c(model, descriptor->animation);
    const void *source = &descriptor->resource_animation;
    void *destination = &model->animation_id;
    if (source < destination)
        func_0202cd2c(source, destination, model->unk_080 - (u8 *)destination);
    else
        func_0202cc58(source, destination, model->unk_080 - (u8 *)destination);
    /* The descriptor stores bytes; the palette allocator takes colors. */
    u16 colors = descriptor->palette_bytes >> 1;
    model->set_primary_animation((u8)descriptor->resource_animation, descriptor->animation_id, 1);
    int speed = model->effect_scale;
    if (speed <= 0)
        speed = -speed;
    model->anchor_offset += speed;
    u32 size = descriptor->texture_tile_count;
    if (!size) {
        if (model->resource_flag_bits.alternate_resource == 1)
            size = model->resource->alternate_tile_counts;
        else
            size = model->resource->normal_tile_counts;
        u32 color256 = model->resource->flags.bits.color256;
        size >>= 10 * color256;
        size &= 1023;
        size <<= color256 + 5;
    } else {
        size <<= 5;
    }
    model->texture_allocation_result = GameTextureAllocation_Allocate(&model->render_texture,
        (u8)descriptor->allocation_flags.texture_allocation, size,
        descriptor->resource_flags.shared_texture, descriptor->primary_id, 0,
        (u8)model->resource_flag_bits.alternate_resource, descriptor->first_texture_tile);
    model->palette_allocation_result = GameTexturePalette_Allocate(&model->render_palette,
        (u8)descriptor->allocation_flags.palette_allocation, (u8)model->resource->flags.bits.texture_format,
        colors, descriptor->resource_flags.shared_palette, 1, descriptor->palette_slot,
        descriptor->secondary_data, descriptor->secondary_id, descriptor->palette_offset);
    model->palette_index = 0;
    if (model->texture_allocation_result == 3)
        model->render_texture.flags.raw &= ~4;
    else
        model->render_texture.flags.raw |= 4;
    if (model->palette_allocation_result == 1)
        model->render_palette.flags &= ~0x20;
    if (BattleRenderModel_GetTextureConversionSize(descriptor->animation,
            model->resource_flag_bits.alternate_resource))
        model->render_texture_offsets = descriptor->texture_offsets;
    else
        model->render_texture_offsets = 0;
    if (model->render_texture_offsets && !descriptor->resource_flags.offsets_ready) {
        if (model->resource_flag_bits.alternate_resource == 1)
            GameGraphics_BuildTextureOffsets((u16 *)model->render_texture_offsets, model->resource);
        else
            GameGraphics_BuildGroupTextureOffsets((u16 *)model->render_texture_offsets, model->resource, -1);
    } else if (!model->render_texture_offsets) {
        model->render_texture_offsets = BattleModel_FindTextureOffsets(3,
            model->resource_flag_bits.alternate_resource, model->resource);
    }
    if (model->animation_id >= model->unknown_40())
        model->animation_id = 0;
    if (model->property_056 > func_02009224(model, -1))
        model->property_056 = 0;
}

int BattleRenderModel_GetTextureConversionSize(const GameGraphicsResource *resource, u32 alternate)
{
    return BattleModel_GetTextureConversionSize(3, (u8)alternate, resource);
}

int BattleRenderModel_RestoreController(BattleModel *model, const void *descriptor, void *controller, s16 animation)
{
    return model->restore_controller(descriptor, controller, animation);
}

void BattleRenderModel_ReleaseResources(BattleModel *model)
{
    GameTextureAllocation_Unlink(&model->render_texture);
    GameTexturePalette_Unlink(&model->render_palette);
}
}
