/* Sub-screen adapter for the three-part renderer, 0x020B7448-0x020B7544. */
#include <game/battle_three_part.h>
#include <game/sprite_output.h>

extern "C" GameOamEntry data_02060740[];

extern "C" void BattleThreePart_DrawSubscreen(BattleModel *model,
    const MtxFx44 *transform, int y_offset)
{
    u8 objects = data_0205a05c[1];
    u8 affines = data_0205a060[1];
    int x = transform->_30 / 256;
    int y = y_offset + transform->_31 / 256;
    u8 previous_objects = objects;
    u8 previous_affines = affines;
    s16 *affine = (s16 *)model->unk_06c;

    // Translation uses Q8 pixels; the four affine coefficients use Q8 from Q12.
    // Signed division must truncate toward zero, including negative values.
    model->animation_offset_x = x;
    model->animation_offset_y = y;
    model->render_anchor_z = transform->_32;
    affine[0] = transform->_00 / 16;
    affine[1] = transform->_10 / 16;
    affine[2] = transform->_01 / 16;
    affine[3] = transform->_11 / 16;
    model->draw(data_02060740, &objects, &affines);
    // The renderer advances local counts; only the resulting group is committed.
    GameOam_AddGroup(1, transform->_32, objects - previous_objects,
        affines - previous_affines);
}
