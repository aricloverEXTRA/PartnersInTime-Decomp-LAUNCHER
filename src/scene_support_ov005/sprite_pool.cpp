/*
 * Overlay 5 sprite pool and OAM collection (overlay 5, 0x02068B20-0x02068EE0).
 *
 * Pooled sprites on an intrusive list, collected into OAM once per frame.
 * Collection applies the pool's camera offset while it walks the list and
 * restores it afterwards, so sprites are positioned in world coordinates and
 * emitted in screen coordinates.
 */

#include <game/battle_scene.h>
#include <game/overlay005_pools.h>
#include <msl/array.h>
extern "C" {
#include <game/sprite_output.h>
#include <game/heap.h>
#include <nitro/fx.h>
}

/* Scrolling reference applied to a sprite before it is drawn. */
typedef struct Overlay5SpriteCamera {
    u8 unknown_00[20];
    fx32 x, y;
} Overlay5SpriteCamera;

/* One pooled overlay-5 sprite: a sprite allocation, its palette and the
   screen position the draw call uses. */
typedef struct Overlay5Sprite {
    const void *vtable;
    u8 unknown_04[16];
    GameSpriteAllocation allocation;
    GameSpritePalette *palette;
    u8 unknown_30[44];
    s16 x, y;
    u8 unknown_60[228];
    u8 active;
    u8 unknown_145[3];
    const Overlay5SpriteCamera *camera;
    s16 offset_x, offset_y;
} Overlay5Sprite;

typedef struct Overlay5SpriteNode {
    Overlay5Sprite *sprite;
    struct Overlay5SpriteNode *next;
} Overlay5SpriteNode;

/* The pool keeps its entries on two lists: free entries and taken entries. */


typedef char Overlay5SpriteSizeCheck[sizeof(Overlay5Sprite) == 0x150 ? 1 : -1];

/* Only the two slots this overlay calls are named. */
class Overlay5SpriteInterface {
public:
    virtual void reserved00();
    virtual void reserved04();
    virtual void reserved08();
    virtual void reserved0c();
    virtual void reserved10();
    virtual void reserved14();
    virtual void reserved18();
    virtual void finish();
    virtual void reserved20();
    virtual void reserved24();
    virtual void release();
};

extern "C" {

extern Overlay5SpritePool data_ov005_0206a3d8;
extern const void *data_ov005_0206a02c[];

extern void func_0200a4cc(Overlay5Sprite *sprite, GameOamEntry *oam, u8 *entry_count, u8 *affine_count);
extern void *func_02048aac(void *memory, u32 count, u32 size, u32 header,
                           void *(*construct)(void *), void *(*destroy)(void *));
extern void *func_ov005_02068818(void *object);

Overlay5Sprite *func_ov005_02068cc0(Overlay5Sprite *sprite);
void func_ov005_02068ce4(Overlay5SpritePool *pool, u32 count);
void func_ov005_02068d8c(Overlay5SpritePool *pool);
void func_ov005_02068e14(Overlay5SpritePool *pool, Overlay5Sprite *sprite);
void func_ov005_02068e34(Overlay5SpritePool *pool);

Overlay5SpritePool *func_ov005_02068ec8(Overlay5SpritePool *pool) {
    func_ov005_02068d8c(pool);
    return pool;
}

void func_ov005_02068e34(Overlay5SpritePool *pool) {
    u32 index = 0;
    while (index < pool->count - 1) {
        pool->nodes[index].sprite = &pool->sprites[index];
        pool->nodes[index].next = &pool->nodes[index + 1];
        ++index;
    }
    (pool->nodes + pool->count - 1)->sprite = &pool->sprites[pool->count - 1];
    (pool->nodes + pool->count - 1)->next = 0;
    pool->free_list = pool->nodes;
    pool->taken_list = 0;
}

void func_ov005_02068e14(Overlay5SpritePool *pool, Overlay5Sprite *sprite) {
    Overlay5SpriteNode *node = pool->taken_list;
    node->sprite = sprite;
    pool->taken_list = node->next;
    node->next = pool->free_list;
    pool->free_list = node;
}

Overlay5Sprite *func_ov005_02068df0(Overlay5SpritePool *pool) {
    Overlay5SpriteNode *node = pool->free_list;
    Overlay5Sprite *sprite = node->sprite;
    pool->free_list = node->next;
    node->next = pool->taken_list;
    pool->taken_list = node;
    return sprite;
}

void func_ov005_02068d8c(Overlay5SpritePool *pool) {
    if (pool->sprites) {
        MSL_DeleteArray(pool->sprites, sizeof(Overlay5Sprite), 8, func_ov005_02068818);
        pool->sprites = 0;
    }
    if (pool->nodes) {
        GameHeap_DeleteArray(pool->nodes);
        pool->nodes = 0;
    }
    pool->sprites = 0;
    pool->nodes = 0;
    pool->free_list = 0;
    pool->taken_list = 0;
}

void func_ov005_02068ce4(Overlay5SpritePool *pool, u32 count) {
    if (pool->sprites || pool->nodes) func_ov005_02068d8c(pool);
    pool->sprites = (Overlay5Sprite *)func_02048aac(
        GameHeap_NewArray(count * sizeof(Overlay5Sprite) + 8, 0, 0, 1), count,
        sizeof(Overlay5Sprite), 8,
        (void *(*)(void *))func_ov005_02068cc0, func_ov005_02068818);
    pool->nodes = (Overlay5SpriteNode *)GameHeap_NewArray(
        count * sizeof(Overlay5SpriteNode), 0, 0, 1);
    pool->count = count;
    func_ov005_02068e34(pool);
}

Overlay5Sprite *func_ov005_02068cc0(Overlay5Sprite *sprite) {
    BattleModelController_Init((BattleModel *)sprite);
    sprite->vtable = data_ov005_0206a02c;
    return sprite;
}

void func_ov005_02068ca8(u32 count) {
    func_ov005_02068ce4(&data_ov005_0206a3d8, count);
}

void func_ov005_02068c94(void) {
    func_ov005_02068d8c(&data_ov005_0206a3d8);
}

Overlay5Sprite *func_ov005_02068c80(void) {
    return func_ov005_02068df0(&data_ov005_0206a3d8);
}

void func_ov005_02068c54(Overlay5Sprite *sprite) {
    ((Overlay5SpriteInterface *)sprite)->finish();
    func_ov005_02068e14(&data_ov005_0206a3d8, sprite);
}

void func_ov005_02068c1c(Overlay5Sprite *sprite, int mode) {
    BattleModelController_Restore((BattleModel *)sprite, (const void *)mode, 0, 0);
    sprite->active = 1;
    sprite->offset_x = 0;
    sprite->offset_y = 0;
    sprite->camera = 0;
}

void func_ov005_02068bc4(Overlay5Sprite *sprite) {
    sprite->active = 0;
    if (sprite->palette) {
        GameSpritePalette_Unlink(sprite->palette);
        sprite->palette = 0;
    }
    if (sprite->allocation.flags.bits.linked) {
        GameSpriteAllocation_Unlink(&sprite->allocation);
    }
    ((Overlay5SpriteInterface *)sprite)->release();
}

u32 func_ov005_02068bbc(void) {
    return sizeof(Overlay5Sprite);
}

void Overlay5Sprite_CollectOam(Overlay5Sprite *sprite, GameOamEntry *oam, u8 *entry_count, u8 *affine_count)
{
    if (!sprite->active)
        return;
    s16 x = sprite->x;
    s16 y = sprite->y;
    sprite->x += sprite->offset_x;
    sprite->y += sprite->offset_y;
    if (sprite->camera) {
        sprite->x -= sprite->camera->x >> 12;
        sprite->y -= sprite->camera->y >> 12;
    }
    func_0200a4cc(sprite, oam, entry_count, affine_count);
    sprite->x = x;
    sprite->y = y;
}

}
