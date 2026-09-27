#ifndef PIT_SHOP_OWNED_COUNT_INTERNAL_H
#define PIT_SHOP_OWNED_COUNT_INTERNAL_H

#include <nitro.h>

/* The moving panel and its label/digit children share 72-byte task slots.
 * Children read the parent's Q12 position; target_x is reused as a digit index.
 */
typedef struct ShopOwnedCountTask {
    u8 unknown_00[16];
    struct ShopOwnedCountTask *parent;
    u8 unknown_14[12];
    int phase, frames;
    int target_x, target_y, x, y, velocity_x, unknown_3c, acceleration_x;
    u8 unknown_44[4];
} ShopOwnedCountTask;
typedef char ShopOwnedCountTaskSizeCheck[sizeof(ShopOwnedCountTask) == 72 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
extern u8 data_ov009_0207ea3c[];
void ShopOwnedCount_RequestClose(void);
void ShopOwnedCount_DrawLabel(ShopOwnedCountTask *task);
#ifdef __cplusplus
}
#endif

/* 1 keeps the parent open, 0 requests its exit, 2 retires its children. */
#define SHOP_OWNED_COUNT_PHASE (*(s8 *)(data_ov009_0207ea3c + 0xae))

#endif
