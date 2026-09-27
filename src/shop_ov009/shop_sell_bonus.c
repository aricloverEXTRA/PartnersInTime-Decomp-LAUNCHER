/* Stache-based selling-price bonus (overlay 9, 0x0206D084-0x0206D0BC). */
#include <game/shop_scene.h>

u32 func_ov009_0206d0bc(ShopSceneTask *scene, u8 factor);

/* Selling uses twice the Stache discount, with a maximum bonus of 99%. */
u32 ShopScene_GetSellBonus(ShopSceneTask *scene, u8 factor)
{
    u32 bonus = 200 * func_ov009_0206d0bc(scene, factor) / 100u;
    if (bonus >= 100)
        bonus = 99;
    return bonus;
}
