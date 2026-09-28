/*
 * Overlay 5 item pool and object sprites (overlay 5, 0x020695CC-0x020699F0).
 *
 * A pool of items linked into lists, plus the initializer that turns a pooled
 * item into an object sprite. Clearing uses the 16-bit fill helpers because the
 * targets are VRAM-backed.
 */

#include <nitro.h>
#include <game/overlay005_pools.h>
#include <game/heap.h>
#include <msl/array.h>
#include <game/overlay005_resource.h>

/* A pooled 64-byte overlay-5 item. */
typedef struct Overlay5Item {
    void **vtable;
    u8 unknown_04[12];
    u8 screen;
    u8 unknown_11[3];
    u8 cleared_14[8];
    u32 unknown_1c;
    u32 unknown_20;
    u32 unknown_24;
    u32 unknown_28;
    u16 unknown_2c, unknown_2e;
    u8 active;
    u8 unknown_31[3];
    u32 unknown_34;
    u8 unknown_38[8];
} Overlay5Item;

typedef struct Overlay5ItemLink {
    Overlay5Item *item;
    struct Overlay5ItemLink *next;
} Overlay5ItemLink;

typedef char Overlay5ItemSizeCheck[sizeof(Overlay5Item) == 0x40 ? 1 : -1];

extern Overlay5ItemPool data_ov005_0206aa18;
extern void **data_ov005_0206aa2c;
extern u32 data_ov005_0206aa30;
extern void *data_ov005_0206a120[];

extern void MI_CpuFill8(void *destination, u8 value, u32 size);
extern void MIi_CpuClear16(u16 data, void *destination, u32 size);
static inline void MI_CpuFill16(void *destination, u16 data, u32 size) {
    MIi_CpuClear16(data, destination, size);
}
static inline void MI_CpuClear16(void *destination, u32 size) {
    MI_CpuFill16(destination, 0, size);
}

extern void func_02009138(Overlay5Item *item);
extern void func_02009148(Overlay5Item *item);
extern void *func_02048aac(void *memory, u32 count, u32 size, u32 header,
                           void *(*construct)(void *), void *(*destroy)(void *));
extern void func_ov005_02069a54(void *entry);

Overlay5Item *func_ov005_0206968c(Overlay5Item *item);
Overlay5Item *func_ov005_0206988c(Overlay5Item *item);
void func_ov005_020696b0(Overlay5ItemPool *pool, u32 count);
void func_ov005_02069754(Overlay5ItemPool *pool);
Overlay5Item *func_ov005_020697b8(Overlay5ItemPool *pool);
void func_ov005_020697dc(Overlay5ItemPool *pool, Overlay5Item *item);
void func_ov005_020697fc(Overlay5ItemPool *pool);
void func_ov005_02069928(void);
void func_ov005_020698f0(u32 index);
void *func_ov005_020698dc(u32 index);

void func_ov005_02069998(u32 count) {
    func_ov005_02069928();
    data_ov005_0206aa30 = count;
    /* The original clamps the element count the way an array allocation does. */
    data_ov005_0206aa2c = GameHeap_NewArray((count & 0x3fffffff) * sizeof(void *), 0, 0, 1);
    MI_CpuFill8(data_ov005_0206aa2c, 0, data_ov005_0206aa30 * sizeof(void *));
}

void func_ov005_02069928(void) {
    if (data_ov005_0206aa2c) {
        u32 index = 0;
        while (index < data_ov005_0206aa30) {
            func_ov005_020698f0(index);
            ++index;
        }
        GameHeap_DeleteArray(data_ov005_0206aa2c);
    }
    data_ov005_0206aa2c = 0;
}

void func_ov005_020698f0(u32 index) {
    void *entry = func_ov005_020698dc(index);
    if (!entry) return;
    func_ov005_02069a54(entry);
    data_ov005_0206aa2c[index] = 0;
}

void *func_ov005_020698dc(u32 index) {
    return data_ov005_0206aa2c[index];
}

void func_ov005_020698c8(u32 index, void *entry) {
    data_ov005_0206aa2c[index] = entry;
}

Overlay5ItemPool *func_ov005_020698b0(Overlay5ItemPool *pool) {
    func_ov005_02069754(pool);
    return pool;
}

Overlay5Item *func_ov005_0206988c(Overlay5Item *item) {
    item->vtable = data_ov005_0206a120;
    func_02009138(item);
    return item;
}

void func_ov005_020697fc(Overlay5ItemPool *pool) {
    u32 index = 0;
    while (index < pool->count - 1) {
        pool->links[index].item = &pool->items[index];
        pool->links[index].next = &pool->links[index + 1];
        ++index;
    }
    (pool->links + pool->count - 1)->item = &pool->items[pool->count - 1];
    (pool->links + pool->count - 1)->next = 0;
    pool->free_list = pool->links;
    pool->taken_list = 0;
}

void func_ov005_020697dc(Overlay5ItemPool *pool, Overlay5Item *item) {
    Overlay5ItemLink *link = pool->taken_list;
    link->item = item;
    pool->taken_list = link->next;
    link->next = pool->free_list;
    pool->free_list = link;
}

Overlay5Item *func_ov005_020697b8(Overlay5ItemPool *pool) {
    Overlay5ItemLink *link = pool->free_list;
    Overlay5Item *item = link->item;
    pool->free_list = link->next;
    link->next = pool->taken_list;
    pool->taken_list = link;
    return item;
}

void func_ov005_02069754(Overlay5ItemPool *pool) {
    if (pool->items) {
        MSL_DeleteArray(pool->items, sizeof(Overlay5Item), 8,
                      (void *(*)(void *))func_ov005_0206988c);
        pool->items = 0;
    }
    if (pool->links) {
        GameHeap_DeleteArray(pool->links);
        pool->links = 0;
    }
    pool->items = 0;
    pool->links = 0;
    pool->free_list = 0;
    pool->taken_list = 0;
}

void func_ov005_020696b0(Overlay5ItemPool *pool, u32 count) {
    if (pool->items || pool->links) func_ov005_02069754(pool);
    pool->items = func_02048aac(
        GameHeap_NewArray(count * sizeof(Overlay5Item) + 8, 0, 0, 1), count,
        sizeof(Overlay5Item), 8, (void *(*)(void *))func_ov005_0206968c,
        (void *(*)(void *))func_ov005_0206988c);
    pool->links = GameHeap_NewArray(count * sizeof(Overlay5ItemLink), 0, 0, 1);
    pool->count = count;
    func_ov005_020697fc(pool);
}

Overlay5Item *func_ov005_0206968c(Overlay5Item *item) {
    func_02009148(item);
    item->vtable = data_ov005_0206a120;
    return item;
}

void func_ov005_02069674(u32 count) {
    func_ov005_020696b0(&data_ov005_0206aa18, count);
}

void func_ov005_02069660(void) {
    func_ov005_02069754(&data_ov005_0206aa18);
}

Overlay5Item *func_ov005_0206964c(void) {
    return func_ov005_020697b8(&data_ov005_0206aa18);
}

void Overlay5ObjectSprite_Init(Overlay5ObjectSprite *sprite, int screen)
{
    Overlay5Item *item = (Overlay5Item *)sprite;
    vu16 zero;
    item->active = 1;
    /* The native fill wrapper stores and reloads this stack halfword. */
    zero = 0;
    MI_CpuFill16(item->cleared_14, zero, sizeof(item->cleared_14));
    item->unknown_1c = 0;
    item->unknown_20 = 0;
    item->unknown_2c = 0;
    item->unknown_2e = 0;
    item->unknown_24 = 0;
    item->unknown_28 = 0;
    item->unknown_34 = 0;
    item->screen = screen;
}

void func_ov005_020695cc(void *object)
{
    Overlay5Item *item = (Overlay5Item *)object;
    item->active = 0;
    func_ov005_020697dc(&data_ov005_0206aa18, item);
}
