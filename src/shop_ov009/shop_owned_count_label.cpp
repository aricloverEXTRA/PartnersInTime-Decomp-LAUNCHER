/* Owned-count label strips, overlay 9, 0x02079C08-0x02079C68. */
#include "shop_owned_count_internal.h"
#include "shop_rows_internal.h"

extern "C" void func_ov005_0206650c(void *);

extern "C" void ShopOwnedCount_DrawLabel(ShopOwnedCountTask *task)
{
    ShopOwnedCountTask *parent = task->parent;
    ShopRowSprite *sprite = Overlay5ResourceB_Get(task);
    if (SHOP_OWNED_COUNT_PHASE == 2) {
        func_ov005_0206650c(task);
        return;
    }
    // Preserve the parent's Q12 coordinates as the panel slides in or out.
    sprite->x = parent->x;
    sprite->y = parent->y;
    func_ov005_02069084(sprite, 59);
}
