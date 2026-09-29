#ifndef PIT_GAME_MENU_SPRING_H
#define PIT_GAME_MENU_SPRING_H
#include <nitro.h>

/* Position, previous position and velocity use 12 fractional bits. */
typedef struct MenuSpringPoint {
    s32 x, y, z;
    s32 previous_x, previous_y, previous_z;
    s32 velocity_x, velocity_y, velocity_z;
    s32 gravity_scale; /* 8 fractional bits */
    u16 pinned, unknown_2a;
} MenuSpringPoint;
typedef struct MenuSpringLink {
    MenuSpringPoint *first, *second;
    s32 length;
} MenuSpringLink;
typedef struct MenuSpringChain {
    int state;
    union {
        u8 unknown_04[16];
        struct { int target_x, target_y, step_x, step_y; };
    };
    s32 minimum_y;
    MenuSpringPoint points[4];
    MenuSpringLink links[3];
} MenuSpringChain;

/* Shared 72-byte task used by the party model and its hanging links. */
typedef struct MenuSpringTask {
    u8 unknown_00[32];
    int state, counter;
    u16 chain_index, unknown_2a;
    int x, y, selector, point_index, scale, angle;
    u32 unknown_44;
} MenuSpringTask;

typedef char MenuSpringTaskSizeCheck[sizeof(MenuSpringTask) == 72 ? 1 : -1];
typedef char MenuSpringChainTargetOffsetCheck[
    (u32)&((MenuSpringChain *)0)->target_x == 4 ? 1 : -1];

typedef char MenuSpringPointSizeCheck[sizeof(MenuSpringPoint) == 44 ? 1 : -1];
typedef char MenuSpringLinkSizeCheck[sizeof(MenuSpringLink) == 12 ? 1 : -1];
typedef char MenuSpringChainSizeCheck[sizeof(MenuSpringChain) == 236 ? 1 : -1];

#ifdef __cplusplus
extern "C" {
#endif
void MenuSpring_UpdatePartyModel(MenuSpringTask *task);
void MenuSpring_AdvanceActiveChains(void);
void MenuSpring_Update(MenuSpringChain *chain, int iterations);
void MenuSpring_InitChain(int index, int x, int y);
void MenuSpring_UpdateVelocities(MenuSpringPoint *point, int count);
void MenuSpring_ConstrainLinks(MenuSpringLink *link, int count);
void MenuSpring_IntegratePoints(MenuSpringPoint *point, int count, int minimum_y);
#ifdef __cplusplus
}
#endif
#endif
