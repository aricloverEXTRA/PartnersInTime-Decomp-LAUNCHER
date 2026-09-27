/* Field corner-overlap correction (overlay 0, 0x020AA30C-0x020AA5B0). */

#include <game/field_navigation.h>

#define MINIMUM(a, b) ((a) <= (b) ? (a) : (b))
#define MAXIMUM(a, b) ((a) >= (b) ? (a) : (b))

void FieldNavigation_ResolveCornerOverlap(FieldRuntimeEntity *entity, fx32 *corner_x,
                                          fx32 *corner_y, int iterations)
{
    /* Move one world unit (4096 in Q12) away from a partial obstruction, then rebuild
     * the swept bounds before querying again. Nonpositive limits are unbounded. */
    do {
        int mask = func_ov000_020aa5b0(entity, corner_x, corner_y);
        fx32 dx, dy;
        FieldNavigationSurface *surface;
        if (!mask || mask == 15)
            return;
        dx = 0;
        dy = 0;
        switch (mask) {
        case 1:
        case 11:
            dx = 4096;
            dy = 4096;
            break;
        case 2:
        case 7:
            dx = 4096;
            dy = -4096;
            break;
        case 4:
        case 14:
            dx = -4096;
            dy = -4096;
            break;
        case 8:
        case 13:
            dx = -4096;
            dy = 4096;
            break;
        case 9:
            dy = 4096;
            break;
        case 12:
            dx = -4096;
            break;
        case 6:
            dy = -4096;
            break;
        case 3:
            dx = 4096;
            break;
        }
        if (!dx && !dy)
            return;
        if (dx) {
            corner_x[0] += dx;
            corner_x[1] += dx;
            corner_x[2] += dx;
            corner_x[3] += dx;
            entity->position_x += dx;
        }
        if (dy) {
            corner_y[0] += dy;
            corner_y[1] += dy;
            corner_y[2] += dy;
            corner_y[3] += dy;
            entity->position_y += dy;
        }
        surface = entity->navigation_surfaces;
        if (surface) {
            fx32 minimum_x = MINIMUM(entity->position_x, entity->previous_position_x);
            fx32 limit = entity->navigation_min_x + minimum_x - 0x80000
                         - entity->locomotion.starting_speed;
            while (!surface->bits.end && surface->sort_x < limit)
                ++surface;
        }
        entity->navigation_cursor = surface;
        entity->navigation_scan_limit = entity->navigation_max_x
            + MAXIMUM(entity->position_x, entity->previous_position_x) + 0x10000;
        entity->swept_min_x = entity->navigation_min_x
            + MINIMUM(entity->position_x, entity->previous_position_x)
            - entity->locomotion.starting_speed;
        entity->swept_min_y = entity->navigation_min_y
            + MINIMUM(entity->position_y, entity->previous_position_y)
            - entity->locomotion.starting_speed;
        entity->swept_max_x = entity->locomotion.starting_speed
            + (entity->navigation_max_x + MAXIMUM(entity->position_x, entity->previous_position_x));
        entity->swept_max_y = entity->locomotion.starting_speed
            + (entity->navigation_max_y + MAXIMUM(entity->position_y, entity->previous_position_y));
    } while (iterations <= 0 || --iterations > 0);
}
