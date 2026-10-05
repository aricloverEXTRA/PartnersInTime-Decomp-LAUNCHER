/*
 * Path construction from a script-supplied point list (overlay 0,
 * 0x020B1A24-0x020B1B88).
 *
 * The first two entries of the list are flags rather than offsets: entry 0's X
 * bit is the bounce flag and entry 1 supplies the random-direction flag and the
 * 13-bit delay. The remaining entries are Q12 offsets, one pair per path point.
 *
 * When the relative flag (entry 0's Y) is nonzero the offsets are accumulated
 * from the entity's current position, which is emitted as the first point, so
 * the stored table is absolute; otherwise each offset is stored directly.
 */

#include <game/field_roaming.h>

/* Native merges the flag updates into one 16-bit read-modify-write chain
 * (ldrh/bic/orr/strh) and rewrites the 0x3e4 word through an explicit 0x1F0
 * mask. The shared record exposes typed bitfields at both offsets whose
 * separate spellings do not reproduce that, so read them through raw views
 * pinned to the same layout instead. */
typedef struct FieldPathFlagView {
    u16 flags;
    u16 unknown_02;
    fx32 points[64];
} FieldPathFlagView;
typedef char FieldPathFlagView_SizeCheck[sizeof(FieldPathFlagView) == 0x104 ? 1 : -1];
typedef struct FieldPathCountView {
    u32 state;
} FieldPathCountView;
typedef char FieldPathCountView_Pin[
    (u32)&((FieldRuntimeEntity *)0)->path_state == 0x3E4 ? 1 : -1];

void FieldPath_Setup(FieldRuntimeEntity *entity, const FieldPathPoint *list, int count)
{
    FieldPathFlagView *view = (FieldPathFlagView *)&entity->unknown_3ec;
    fx32 *point;
    u32 pos_x, pos_y;
    u32 index;
    s32 relative;
    s32 i;

    relative = list[0].y;
    view->flags = (view->flags & ~1) | 1;
    view->flags = (view->flags & ~2) | (((u16)list[0].x & 1) << 1);
    view->flags = (view->flags & ~4) | (((u16)list[1].x & 1) << 2);
    view->flags = (view->flags & ~0xFFF8) | (((u16)list[1].y & 0x1FFF) << 3);
    ((FieldPathCountView *)&entity->path_state)->state =
        (((FieldPathCountView *)&entity->path_state)->state & ~0x1F0) | ((count & 0x1F) << 4);

    list += 2;
    point = entity->unknown_3ec.path.coordinates;

    if (relative == 0) {
        for (i = 0; i < count; i++) {
            point[0] = list->x << 12;
            point[1] = list->y << 12;
            list++;
            point += 2;
        }
    }
    else {
        pos_x = entity->position_x;
        pos_y = entity->position_y;
        point[0] = pos_x;
        point[1] = pos_y;
        point += 2;
        for (i = 0; i < count; i++) {
            pos_x += list->x << 12;
            pos_y += list->y << 12;
            point[0] = pos_x;
            point[1] = pos_y;
            list++;
            point += 2;
        }
        index = (((FieldPathCountView *)&entity->path_state)->state << 23) >> 27;
        index = index + 1;
        ((FieldPathCountView *)&entity->path_state)->state =
            (((FieldPathCountView *)&entity->path_state)->state & ~0x1F0) | ((index & 0x1F) << 4);
    }
}