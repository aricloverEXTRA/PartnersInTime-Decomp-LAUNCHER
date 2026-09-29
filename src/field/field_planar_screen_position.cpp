/* Screen coordinates and render ordering for planar field entities.
 * Their coordinates are already in screen space, so camera arguments are unused.
 * Accesses stay within the shared 688-byte planar entity allocation. */
#include <game/field_entity.h>
extern "C" {

void FieldEntity2D_UpdateScreenPosition(FieldRuntimeEntity *entity, s16 camera_x, s16 camera_y)
{
    if (entity->base_state_flag_bits.animation_wait_enabled) {
        FieldRenderObject *model;
        entity->screen_x = entity->position_x / 4096;
        entity->screen_y = entity->position_y / 4096;
        model = entity->render_object;
        if (model) {
            /* Both Y values are read before either renderer offset is stored. */
            int screen_y = entity->screen_y;
            int offset_y = entity->screen_offset_y;
            model->animation_offset_x = entity->screen_x + entity->screen_offset_x;
            model->animation_offset_y = screen_y + offset_y;
            entity->render_object->field_sort_key.vertical_order =
                8192 - (entity->position_y + entity->interaction_min_y) / 4096;
            entity->render_object->field_sort_key.entity_index = entity->base.index;
        }
    }
}
}
