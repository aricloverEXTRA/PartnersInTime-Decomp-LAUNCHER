#ifndef PIT_CORE_PIT_H
#define PIT_CORE_PIT_H

#define PIT_SCREEN_WIDTH  256
#define PIT_SCREEN_HEIGHT 192
#define PIT_SCREEN_COUNT  2

typedef unsigned int pit_pixel;

typedef struct {
    pit_pixel data[PIT_SCREEN_HEIGHT][PIT_SCREEN_WIDTH];
} pit_screen;

typedef struct {
    pit_screen screen[PIT_SCREEN_COUNT];
} pit_frame;

typedef enum {
    PIT_SCREEN_ENGINE = 0,
    PIT_SCREEN_ACTION = 1
} pit_screen_id;

typedef enum {
    PIT_LAYOUT_STACKED = 0,
    PIT_LAYOUT_SIDE_BY_SIDE,
    PIT_LAYOUT_OVERLAY,
    PIT_LAYOUT_COUNT
} pit_layout_mode;

typedef enum {
    PIT_FOCUS_AUTO = 0,
    PIT_FOCUS_FORCE_ENGINE,
    PIT_FOCUS_FORCE_ACTION,
    PIT_FOCUS_COUNT
} pit_focus_mode;

typedef struct {
    pit_layout_mode layout;
    pit_screen_id  focus;
    int            scale;
} pit_present_info;

#define PIT_ARGB(a, r, g, b) \
    (((unsigned)(a) << 24) | ((unsigned)(r) << 16) | ((unsigned)(g) << 8) | (unsigned)(b))

#endif
