/*
 * Windows patcher front end.
 *
 * The UI is drawn into a low-resolution pit_image and blown up with integer
 * scaling and nearest-neighbour filtering, so every pixel stays square. That is
 * the whole point of the look: the text is the same 8x8 font the Android build
 * uses, so the two front ends are pixel-identical apart from the platform
 * chrome.
 *
 * Patching runs on a worker thread. The log callback it invokes is the same one
 * the headless driver and the Android build use; the only addition here is a
 * mutex, because the callback arrives from the worker while the main loop is
 * repainting.
 *
 * The UI is split into tabs. PATCH is the core flow (verify + prepare, with the
 * plan applied when the MODS toggles it on). MODS holds the optional data mods,
 * and ABOUT explains what the tool is for: preparing the user's own EUR ROM for
 * the Partners in Time decompilation project, which is only partially
 * reconstructed and may not boot yet. Keyboard support mirrors the Android
 * build: Tab cycles focus, arrow keys change tabs, Enter/Space activate.
 */

#include <SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/pit_gfx.h"
#include "core/pit_patch_data.h"
#include "core/pit_patcher.h"
#include "core/pit_png.h"
#include "platform/patcher_ui.h"

/* ------------------------------------------------------------------ palette
 *
 * Every colour below is taken from the real EUR ROM, not invented. GBA
 * palettes are 15-bit BGR555 (bits 10-14 red, 5-9 green, 0-4 blue), expanded
 * to 8 bits per channel with c * 255 / 31. Sources are 256-colour records
 * found in the cartridge and confirmed by their 0x7C1F transparent index:
 *
 *   ROM 0x01C7420  menu UI record: deep-navy and blue-grey ramp at 1-15,
 *                  deep blue at 16-54. Supplies the whole neutral structure.
 *   ROM 0x13661E8  gold ramp, 0x04D12FC brighter gold. The signature PiT
 *                  highlight colour.
 *   ROM 0x0810934  green ramp (HP, confirmations).
 *   ROM 0x0471400  red ramp (damage, errors).
 */

#define C_BG        PIT_ARGB(255, 0x00, 0x10, 0x29)  /* ROM 0x01C7420[35] deep navy */
#define C_BG_TOP    PIT_ARGB(255, 0x00, 0x08, 0x14)  /* darker than ROM, backdrop */
#define C_PANEL     PIT_ARGB(255, 0x00, 0x10, 0x6B)  /* ROM 0x01C7420[16] */
#define C_PANEL_HI  PIT_ARGB(255, 0x00, 0x1C, 0x94)  /* ROM 0x01C7420[18] */
#define C_PANEL_LO  PIT_ARGB(255, 0x00, 0x0B, 0x4A)  /* ROM 0x01C7420[17] */
#define C_WELL      PIT_ARGB(255, 0x00, 0x09, 0x33)  /* ROM 0x01C7420[21] inset */
#define C_EDGE      PIT_ARGB(255, 0x52, 0x63, 0x7B)  /* ROM 0x01C7420[8]  blue grey */
#define C_EDGE_SOFT PIT_ARGB(255, 0x1B, 0x2A, 0x94)  /* ROM 0x01C7420[11] */
#define C_EDGE_HOT  PIT_ARGB(255, 0xFF, 0xA5, 0x21)  /* ROM 0x04D12FC bright gold */
#define C_TEXT      PIT_ARGB(255, 0xC5, 0xDE, 0xF7)  /* ROM 0x01C7420[15] pale blue */
#define C_TEXT_HI   PIT_ARGB(255, 0xFF, 0xFF, 0xFF)  /* ROM 0x01C7420[31] white */
#define C_DIM       PIT_ARGB(255, 0x73, 0x84, 0x94)  /* ROM 0x01C7420[9]  */
#define C_ACCENT    PIT_ARGB(255, 0xDE, 0x94, 0x29)  /* ROM 0x13661E8[60] gold */
#define C_ACCENT_LO PIT_ARGB(255, 0x9A, 0x63, 0x10)  /* ROM 0x13661E8[32] */
#define C_AMBER     PIT_ARGB(255, 0xFF, 0x94, 0x21)  /* ROM 0x04D12FC[2]  bright gold */
#define C_AMBER_HI  PIT_ARGB(255, 0xFF, 0xD2, 0x7A)  /* ROM 0x04D12FC[5]  */
#define C_PILL_TEXT PIT_ARGB(255, 0x2A, 0x18, 0x02)  /* on gold, dark enough */
#define C_ERROR     PIT_ARGB(255, 0xEF, 0x5A, 0x63)  /* ROM 0x03BFA00 soft red */
#define C_OK        PIT_ARGB(255, 0x52, 0xC5, 0x5A)  /* ROM 0x0810934 green */
#define C_SHADOW    PIT_ARGB(160, 0x00, 0x00, 0x08)  /* ROM 0x01C7420[21] */
#define C_SHADOW_SOFT PIT_ARGB(96, 0x00, 0x00, 0x04)  /* spread shadow */
#define C_TRACK     PIT_ARGB(255, 0x00, 0x06, 0x1F)  /* progress track */

#define UI_W 480
#define UI_H 320
#define FONT_W 8
#define FONT_H 8
#define LINE_H 10

/* Layout. Every number render() and setup_buttons() use lives here so the hit
 * tests cannot drift from what is drawn. The Android View mirrors every one of
 * these names and values exactly; tools/check_ui_parity.py proves that. */
#define MARGIN        12
#define HEADER_H      48

#define TAB_X0        MARGIN
#define TAB_Y         50
#define TAB_H         16
#define TAB_COUNT     3
#define TAB_GAP       8
#define TAB_W         ((UI_W - 2 * MARGIN - (TAB_COUNT - 1) * TAB_GAP) / TAB_COUNT)
#define CONTENT_Y     76

#define FIELD_X       76
#define FIELD_W       (UI_W - FIELD_X - 12 - 78)
#define FIELD_H       16
#define FIELD_Y0      88
#define FIELD_DY      28
#define BROWSE_X      (UI_W - MARGIN - 76)
#define BROWSE_W      76
#define BROWSE_H      20

#define MODS_Y        146   /* plan status + keyboard hints on the PATCH tab */

#define PATCH_X       MARGIN
#define PATCH_Y       156
#define PATCH_W       160
#define PATCH_H       26
#define STATUS_X      (PATCH_X + PATCH_W + 14)
#define BAR_X         MARGIN
#define BAR_Y         190
#define BAR_H         12
#define BAR_W         (UI_W - 2 * MARGIN)

#define LOG_X         MARGIN
#define LOG_Y         212
#define LOG_W         (UI_W - 2 * MARGIN)
#define LOG_H         (FOOTER_Y - 8 - LOG_Y)

#define FOOTER_Y      308

#define MODS_CARD_X   MARGIN
#define MODS_CARD_Y   84
#define MODS_CARD_W   (UI_W - 2 * MARGIN)
#define MODS_CARD_H   160
#define MODS_HINT_Y   (MODS_CARD_Y + MODS_CARD_H + 10)
#define TOGGLE_X      (MODS_CARD_X + 16)
#define TOGGLE_Y      112
#define TOGGLE_W      32
#define TOGGLE_H      14
#define TOGGLE_KNOB_W 12
#define MODS_NAME_X   (TOGGLE_X + TOGGLE_W + 8)
#define MODS_NAME_Y   (TOGGLE_Y - 4)
#define MODS_DESC_Y   130
#define MODS_DIV_Y    152

#define CHIP_H        24
#define CHIP_GAP      12
#define CHIP_COUNT    3
#define CHIP_W        ((MODS_CARD_W - 24 - (CHIP_COUNT - 1) * CHIP_GAP) / CHIP_COUNT)
#define CHIP_X0       (MODS_CARD_X + 12)
#define CHIP_Y0       (MODS_DIV_Y + 8)
#define CHIP_DY       (CHIP_H + 4)

#define ABOUT_CARD_X  MARGIN
#define ABOUT_CARD_Y  84
#define ABOUT_CARD_W  (UI_W - 2 * MARGIN)
#define ABOUT_CARD_H  (FOOTER_Y - 8 - ABOUT_CARD_Y)

/* --------------------------------------------------------------------- logs */

#define LOG_MAX        64

typedef enum {
    LOG_INFO = 0,
    LOG_OK,
    LOG_WARN,
    LOG_ERROR
} log_level;

typedef struct {
    char text[128];
    log_level level;
} log_line;

typedef struct {
    log_line line[LOG_MAX];
    int count;
} log_buffer;

/* ---------------------------------------------------------------- ui_widgets */

typedef enum {
    TAB_PATCH = 0,
    TAB_MODS,
    TAB_ABOUT
} tab_id;

typedef enum {
    BTN_NONE = 0,
    BTN_TAB_PATCH,
    BTN_TAB_MODS,
    BTN_TAB_ABOUT,
    BTN_TOGGLE,
    BTN_BROWSE_IN,
    BTN_BROWSE_OUT,
    BTN_PATCH
} button_id;

/* Keyboard focus slots. Fields have their own slots so Tab can move through
 * the wells as well as the buttons; a field slot also means "currently being
 * typed into". */
#define FOCUS_NONE       0
#define FOCUS_SRC        1
#define FOCUS_BROWSE_IN  2
#define FOCUS_OUT        3
#define FOCUS_BROWSE_OUT 4
#define FOCUS_PATCH      5
#define FOCUS_TOGGLE     6

#define BTN_COUNT        7

static const char *level_prefix(log_level level)
{
    switch (level) {
    case LOG_OK: return "+ ";
    case LOG_WARN: return "! ";
    case LOG_ERROR:  return "X ";
    default:       return "  ";
    }
}

static pit_pixel level_color(log_level level)
{
    switch (level) {
    case LOG_OK: return C_OK;
    case LOG_WARN: return C_AMBER;
    case LOG_ERROR:  return C_ERROR;
    default:       return C_TEXT;
    }
}

static int font_char_index(char c)
{
    int index = (int)(unsigned char)c - 0x20;

    if (index < 0 || index >= (int)PIT_FONT_GLYPH_COUNT) {
        return -1;
    }
    return index;
}

static int text_width(const char *text, int scale)
{
    return (int)strlen(text) * FONT_W * scale;
}

static void put_pixel(pit_image *img, int x, int y, pit_pixel color)
{
    if (x < 0 || y < 0 || x >= img->width || y >= img->height) {
        return;
    }
    img->pixels[y * img->width + x] = color;
}

/* Draws text, skipping characters outside the font's range. */
static void draw_text(pit_image *img, int x, int y, pit_pixel color,
                      const char *text, int scale)
{
    for (; *text; text++) {
        int index = font_char_index(*text);
        int row;
        int col;

        if (index >= 0) {
            for (row = 0; row < 8; row++) {
                unsigned char bits = PIT_FONT8X8[index][row];

                for (col = 0; col < 8; col++) {
                    if (bits & (unsigned char)(1u << (7 - col))) {
                        int px = x + col * scale;
                        int py = y + row * scale;
                        int sy;
                        int sx;

                        for (sy = 0; sy < scale; sy++) {
                            for (sx = 0; sx < scale; sx++) {
                                put_pixel(img, px + sx, py + sy, color);
                            }
                        }
                    }
                }
            }
        }
        x += FONT_W * scale;
    }
}

static void draw_text_clipped(pit_image *img, int x, int y, pit_pixel color,
                              const char *text, int scale, int max_width)
{
    char buffer[128];
    int room = max_width / (FONT_W * scale);
    size_t len = strlen(text);

    if (room <= 0) {
        return;
    }
    if (len > (size_t)room) {
        if (room > 3) {
            len = (size_t)(room - 3);
        } else {
            len = (size_t)room;
        }
        memcpy(buffer, text, len);
        memcpy(buffer + len, "...", 4);
    } else {
        memcpy(buffer, text, len + 1);
    }
    draw_text(img, x, y, color, buffer, scale);
}

static void fill_rect(pit_image *img, int x, int y, int w, int h, pit_pixel c)
{
    int row;
    int col;

    for (row = y; row < y + h; row++) {
        for (col = x; col < x + w; col++) {
            put_pixel(img, col, row, c);
        }
    }
}

static int clampi(int v, int lo, int hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

/* Source-over blend. Kept in integer arithmetic so the result is identical on
 * every host, which matters because the headless screenshot is compared against
 * the on-screen frame. Channels are shifted down to 0-255 before scaling;
 * multiplying the masked word would scale it by 256 and overflow the field. */
static void blend_pixel(pit_image *img, int x, int y, pit_pixel src)
{
    unsigned int sa = (src >> 24) & 0xFFu;
    pit_pixel dst;
    unsigned int r;
    unsigned int g;
    unsigned int b;
    unsigned int ia;

    if (sa == 0u) {
        return;
    }
    if (x < 0 || y < 0 || x >= img->width || y >= img->height) {
        return;
    }
    if (sa == 0xFFu) {
        put_pixel(img, x, y, src);
        return;
    }
    dst = img->pixels[y * img->width + x];
    ia = 255u - sa;
    r = (((src >> 16) & 0xFFu) * sa + ((dst >> 16) & 0xFFu) * ia) / 255u;
    g = (((src >> 8) & 0xFFu) * sa + ((dst >> 8) & 0xFFu) * ia) / 255u;
    b = ((src & 0xFFu) * sa + (dst & 0xFFu) * ia) / 255u;
    put_pixel(img, x, y, PIT_ARGB(0xFFu, (int)r, (int)g, (int)b));
}

static pit_pixel lerp_color(pit_pixel a, pit_pixel b, int t)
{
    int r = (int)(((a & 0x00FF0000u) >> 16) * (255 - t) / 255 +
                  ((b & 0x00FF0000u) >> 16) * t / 255);
    int g = (int)(((a & 0x0000FF00u) >> 8) * (255 - t) / 255 +
                  ((b & 0x0000FF00u) >> 8) * t / 255);
    int bb = (int)((a & 0x000000FFu) * (255 - t) / 255 +
                   (b & 0x000000FFu) * t / 255);

    return PIT_ARGB(0xFF, r, g, bb);
}

/* Rounded rectangle, drawn without anti-aliasing so the whole UI stays on the
 * logical pixel grid and the headless screenshot matches the window exactly.
 * Corners are quarter circles; `radius` is clamped to half the shorter side. */
static void fill_round_rect(pit_image *img, int x, int y, int w, int h, int radius,
                            pit_pixel color)
{
    int dx;
    int dy;

    if (w <= 0 || h <= 0) {
        return;
    }
    if (radius * 2 > w) {
        radius = w / 2;
    }
    if (radius * 2 > h) {
        radius = h / 2;
    }
    if (radius <= 0) {
        fill_rect(img, x, y, w, h, color);
        return;
    }

    fill_rect(img, x + radius, y, w - 2 * radius, h, color);
    fill_rect(img, x, y + radius, w, 2 * radius, color);
    for (dy = 0; dy < radius; dy++) {
        for (dx = 0; dx < radius; dx++) {
            int ox = radius - 1 - dx;
            int oy = radius - 1 - dy;

            if (ox * ox + oy * oy <= radius * radius) {
                put_pixel(img, x + dx, y + dy, color);
                put_pixel(img, x + w - 1 - dx, y + dy, color);
            }
            if (ox * ox + dy * dy <= radius * radius) {
                put_pixel(img, x + dx, y + h - 1 - dy, color);
                put_pixel(img, x + w - 1 - dx, y + h - 1 - dy, color);
            }
        }
    }
}

/* Vertical gradient inside a rounded rectangle. Rows are painted one at a time
 * and the corner pixels are skipped by the same inside test fill_round_rect()
 * uses, so no colour from an adjacent row bleeds into the cut corners. */
static void fill_round_gradient(pit_image *img, int x, int y, int w, int h,
                                int radius, pit_pixel top, pit_pixel bottom)
{
    int r = radius;
    int row;
    int dx;

    if (w <= 0 || h <= 0) {
        return;
    }
    if (r * 2 > w) {
        r = w / 2;
    }
    if (r * 2 > h) {
        r = h / 2;
    }

    for (row = 0; row < h; row++) {
        int t = (h <= 1) ? 0 : (row * 255) / (h - 1);
        pit_pixel c = lerp_color(top, bottom, t);
        int x0 = 0;
        int x1 = w;

        if (r > 0) {
            if (row < r) {
                int oy = r - 1 - row;

                for (x0 = 0; x0 < r; x0++) {
                    int ox = r - 1 - x0;

                    if (ox * ox + oy * oy <= r * r) {
                        break;
                    }
                }
            } else if (row >= h - r) {
                int oy = row - (h - r);

                for (x0 = 0; x0 < r; x0++) {
                    int ox = r - 1 - x0;

                    if (ox * ox + oy * oy <= r * r) {
                        break;
                    }
                }
            }
        }
        x1 = w - x0;
        if (x1 > x0) {
            fill_rect(img, x + x0, y + row, x1 - x0, 1, c);
        }
    }
}

/* Rounded rect drawn as a source-over blend, so shadows darken what is already
 * there instead of stamping an opaque block over it. */
static void blend_round_rect(pit_image *img, int x, int y, int w, int h,
                             int radius, pit_pixel color)
{
    int r = radius;
    int dy;
    int dx;

    if (w <= 0 || h <= 0) {
        return;
    }
    if (r * 2 > w) {
        r = w / 2;
    }
    if (r * 2 > h) {
        r = h / 2;
    }
    for (dy = 0; dy < h; dy++) {
        for (dx = 0; dx < w; dx++) {
            int inside = 1;

            if (r > 0) {
                int ox = 0;
                int oy = 0;

                if (dx < r) {
                    ox = r - 1 - dx;
                } else if (dx >= w - r) {
                    ox = dx - (w - r);
                }
                if (dy < r) {
                    oy = r - 1 - dy;
                } else if (dy >= h - r) {
                    oy = dy - (h - r);
                }
                if (ox != 0 || oy != 0) {
                    inside = (ox * ox + oy * oy <= r * r);
                }
            }
            if (inside) {
                blend_pixel(img, x + dx, y + dy, color);
            }
        }
    }
}

/*
 * Outline of a rounded rectangle, one pixel wide. Edges are trimmed back at the
 * corners so the border follows the curve instead of cutting across it.
 */
static void stroke_round_rect(pit_image *img, int x, int y, int w, int h,
                              int radius, pit_pixel color)
{
    int r = radius;
    int dx;
    int dy;

    if (w <= 0 || h <= 0) {
        return;
    }
    if (r * 2 > w) {
        r = w / 2;
    }
    if (r * 2 > h) {
        r = h / 2;
    }
    if (r <= 0) {
        fill_rect(img, x, y, w, 1, color);
        fill_rect(img, x, y + h - 1, w, 1, color);
        fill_rect(img, x, y, 1, h, color);
        fill_rect(img, x + w - 1, y, 1, h, color);
        return;
    }
    fill_rect(img, x + r, y, w - 2 * r, 1, color);
    fill_rect(img, x + r, y + h - 1, w - 2 * r, 1, color);
    fill_rect(img, x, y + r, 1, h - 2 * r, color);
    fill_rect(img, x + w - 1, y + r, 1, h - 2 * r, color);
    for (dy = 0; dy < r; dy++) {
        for (dx = 0; dx < r; dx++) {
            int ox = r - 1 - dx;
            int oy = r - 1 - dy;

            /* Only pixels that belong to the shape are part of its border. */
            if (ox * ox + oy * oy <= r * r) {
                put_pixel(img, x + dx, y + dy, color);
                put_pixel(img, x + w - 1 - dx, y + dy, color);
                put_pixel(img, x + dx, y + h - 1 - dy, color);
                put_pixel(img, x + w - 1 - dx, y + h - 1 - dy, color);
            }
        }
    }
}

/* Soft drop shadow: a few offset passes with falling alpha, blended. */
static void drop_shadow(pit_image *img, int x, int y, int w, int h, int radius)
{
    int spread;

    for (spread = 2; spread >= 1; spread--) {
        int alpha = 44 / spread;
        pit_pixel c = PIT_ARGB(alpha, 0x00, 0x00, 0x06);
        int dy;

        for (dy = 0; dy < spread; dy++) {
            blend_round_rect(img, x - spread + dy, y + dy + spread,
                             w + 2 * (spread - dy), h, radius, c);
        }
    }
}

/*
 * Card: soft shadow, vertical gradient body, bright top bevel, soft border. This
 * is the base every raised surface in the UI is built from.
 */
static void draw_card(pit_image *img, int x, int y, int w, int h, int radius)
{
    drop_shadow(img, x, y, w, h, radius);
    fill_round_gradient(img, x, y, w, h, radius, C_PANEL_HI, C_PANEL_LO);
    /* Top bevel catches the light: a bright 1px strip just inside the border. */
    fill_rect(img, x + radius + 1, y + 1, w - 2 * radius - 2, 1, C_EDGE);
    fill_rect(img, x + radius, y + 1, 1, 1, C_EDGE_SOFT);
    fill_rect(img, x + w - radius - 1, y + 1, 1, 1, C_EDGE_SOFT);
    stroke_round_rect(img, x, y, w, h, radius, C_EDGE_SOFT);
}

/* Inset well for editable text: dark, sunken, with a shadowed top edge. */
static void draw_well(pit_image *img, int x, int y, int w, int h, int active)
{
    fill_round_gradient(img, x, y, w, h, 2, C_PANEL_LO, C_WELL);
    /* Shadowed top edge and a lit bottom edge are what make a well read as
     * inset. The active field swaps the bottom edge for gold. */
    fill_rect(img, x + 2, y, w - 4, 1, PIT_ARGB(150, 0x00, 0x00, 0x08));
    fill_rect(img, x + 2, y + h - 1, w - 4, 1, active ? C_ACCENT : C_EDGE_SOFT);
    stroke_round_rect(img, x, y, w, h, 2, C_EDGE_SOFT);
}

typedef struct {
    button_id id;
    int x, y, w, h;
    const char *label;
    int enabled;
    int hovered;
    int pressed;
    int focus_sel;   /* keyboard focus is on this control this frame */
} ui_button;

static void draw_button(pit_image *img, ui_button *btn)
{
    int primary = (btn->id == BTN_PATCH);
    int radius = 3;
    int hot = btn->hovered || btn->focus_sel;
    pit_pixel top;
    pit_pixel bottom;
    pit_pixel edge;
    pit_pixel label;
    int glyph_scale = primary ? 2 : 1;
    int tx;
    int ty;

    if (primary) {
        if (!btn->enabled) {
            top = C_PANEL;
            bottom = C_PANEL_LO;
            edge = C_EDGE_SOFT;
            label = C_DIM;
        } else if (btn->pressed) {
            top = C_ACCENT_LO;
            bottom = C_ACCENT_LO;
            edge = C_AMBER_HI;
            label = C_TEXT_HI;
        } else if (hot) {
            top = C_AMBER_HI;
            bottom = C_ACCENT;
            edge = C_TEXT_HI;
            label = C_TEXT_HI;
        } else {
            top = C_AMBER;
            bottom = C_ACCENT;
            edge = C_AMBER_HI;
            label = PIT_ARGB(255, 0x2A, 0x18, 0x02);
        }
    } else {
        if (!btn->enabled) {
            top = C_PANEL_LO;
            bottom = C_WELL;
            edge = C_EDGE_SOFT;
            label = C_DIM;
        } else if (btn->pressed) {
            top = C_PANEL_LO;
            bottom = C_WELL;
            edge = C_EDGE;
            label = C_TEXT;
        } else if (hot) {
            top = C_EDGE_HOT;
            bottom = C_ACCENT;
            edge = C_AMBER_HI;
            label = PIT_ARGB(255, 0x2A, 0x18, 0x02);
        } else {
            top = C_PANEL_HI;
            bottom = C_PANEL;
            edge = C_EDGE;
            label = C_TEXT;
        }
    }

    if (!btn->pressed) {
        drop_shadow(img, btn->x, btn->y, btn->w, btn->h, radius);
    } else {
        /* Pressed buttons sit down: the shadow goes under them instead. */
        blend_round_rect(img, btn->x - 1, btn->y + 1, btn->w + 2, btn->h,
                         radius, C_SHADOW_SOFT);
    }

    fill_round_gradient(img, btn->x, btn->y, btn->w, btn->h, radius, top, bottom);

    /* 1px inner highlight along the top edge, then the border. */
    fill_rect(img, btn->x + radius, btn->y + 1, btn->w - 2 * radius, 1,
              PIT_ARGB(90, 0xFF, 0xFF, 0xFF));
    stroke_round_rect(img, btn->x, btn->y, btn->w, btn->h, radius, edge);

    if (primary && btn->enabled) {
        /* Gold bloom on the primary action so it reads as the default. */
        int glow;

        for (glow = 2; glow >= 1; glow--) {
            int step;

            for (step = 0; step < btn->w; step++) {
                int fall = glow * 3;

                if (step < fall || step >= btn->w - fall) {
                    blend_pixel(img, btn->x + step, btn->y - glow,
                                PIT_ARGB(70 / glow, 0xFF, 0xD2, 0x7A));
                    blend_pixel(img, btn->x + step, btn->y + btn->h - 1 + glow,
                                PIT_ARGB(70 / glow, 0xFF, 0xD2, 0x7A));
                }
            }
        }
    }

    tx = btn->x + (btn->w - text_width(btn->label, glyph_scale)) / 2;
    ty = btn->y + (btn->h - FONT_H * glyph_scale) / 2;
    if (primary && !btn->pressed) {
        ty -= 1;  /* 1px drop shadow under the label lifts it off the face */
    }
    draw_text(img, tx + 1, ty + 1, PIT_ARGB(110, 0x00, 0x00, 0x08), btn->label,
              glyph_scale);
    draw_text(img, tx, ty, label, btn->label, glyph_scale);
}

/* Tab strip. The selected tab is gold, like the PATCH button, so the current
 * view reads immediately; the rest sit at panel level until hovered. */
static void draw_tab(pit_image *img, ui_button *btn, int selected)
{
    int hot = btn->hovered || btn->focus_sel;
    int radius = 3;
    pit_pixel top;
    pit_pixel bottom;
    pit_pixel edge;
    pit_pixel label;
    int tx;
    int ty;

    if (selected) {
        top = C_AMBER;
        bottom = C_ACCENT;
        edge = C_AMBER_HI;
        label = C_PILL_TEXT;
    } else if (btn->pressed) {
        top = C_PANEL_LO;
        bottom = C_WELL;
        edge = C_EDGE;
        label = C_TEXT;
    } else if (hot) {
        top = C_PANEL_HI;
        bottom = C_PANEL;
        edge = C_EDGE_HOT;
        label = C_AMBER;
    } else {
        top = C_PANEL_HI;
        bottom = C_PANEL;
        edge = C_EDGE_SOFT;
        label = C_DIM;
    }

    fill_round_gradient(img, btn->x, btn->y, btn->w, btn->h, radius, top, bottom);
    if (selected) {
        fill_rect(img, btn->x + 2, btn->y + 1, btn->w - 4, 1, C_AMBER_HI);
    }
    stroke_round_rect(img, btn->x, btn->y, btn->w, btn->h, radius, edge);

    tx = btn->x + (btn->w - text_width(btn->label, 1)) / 2;
    ty = btn->y + (btn->h - FONT_H) / 2;
    draw_text(img, tx + 1, ty + 1, PIT_ARGB(120, 0x00, 0x00, 0x08), btn->label, 1);
    draw_text(img, tx, ty, label, btn->label, 1);

    /* A gold underline joins the selected tab to the content below it. */
    if (selected) {
        fill_rect(img, btn->x + 8, btn->y + btn->h, btn->w - 16, 2, C_ACCENT);
        fill_rect(img, btn->x + btn->w / 2 - 1, btn->y + btn->h + 2, 2, 1, C_AMBER);
    }
}

/* MODS toggle switch: a small gold/steel track with a sliding knob. */
static void draw_toggle(pit_image *img, ui_button *btn, int on)
{
    int radius = btn->h / 2;
    int hot = btn->hovered || btn->focus_sel;
    int knob = TOGGLE_KNOB_W;
    int kx;
    int ky = btn->y + (btn->h - knob) / 2;

    if (on) {
        fill_round_gradient(img, btn->x, btn->y, btn->w, btn->h, radius,
                            hot ? C_AMBER_HI : C_AMBER, C_ACCENT);
    } else {
        fill_round_gradient(img, btn->x, btn->y, btn->w, btn->h, radius,
                            C_PANEL_LO, C_WELL);
    }
    stroke_round_rect(img, btn->x, btn->y, btn->w, btn->h, radius,
                      (hot || on) ? C_AMBER_HI : C_EDGE_SOFT);

    /* Pressing an ON toggle nudges the knob inward; an OFF toggle pops up. */
    if (btn->pressed) {
        ky += on ? 1 : -1;
    }
    kx = on ? btn->x + btn->w - knob - 2 : btn->x + 2;
    fill_round_gradient(img, kx, ky, knob, knob, 3, C_TEXT_HI, C_TEXT);
    stroke_round_rect(img, kx, ky, knob, knob, 3, C_EDGE);
}

static void draw_progress(pit_image *img, int x, int y, int w, int h,
                          double fraction)
{
    int fill;
    int i;

    if (fraction < 0.0) {
        fraction = 0.0;
    }
    if (fraction > 1.0) {
        fraction = 1.0;
    }

    fill_round_gradient(img, x, y, w, h, 2, C_WELL, C_TRACK);
    fill_rect(img, x + 2, y, w - 4, 1, PIT_ARGB(150, 0x00, 0x00, 0x08));
    stroke_round_rect(img, x, y, w, h, 2, C_EDGE_SOFT);

    /*
     * At rest the bar still shows a short gold stub, so the track never reads as
     * a broken widget before the first run starts.
     */
    if (fraction <= 0.0) {
        fraction = 0.04;
    }
    if (fraction > 1.0) {
        fraction = 1.0;
    }
    fill = (int)((double)(w - 4) * fraction);
    if (fill <= 0) {
        return;
    }
    if (fill > w - 4) {
        fill = w - 4;
    }

    /* Gold gradient, brightest at the leading edge. */
    {
        int rows = h - 2;

        for (i = 0; i < rows; i++) {
            int t = (rows <= 1) ? 0 : (i * 255) / (rows - 1);

            fill_rect(img, x + 2, y + 1 + i, fill, 1, lerp_color(C_AMBER_HI,
                                                                 C_ACCENT, t));
        }
    }
    /* Gloss along the top of the fill. */
    fill_rect(img, x + 2, y + 1, fill, 1, PIT_ARGB(150, 0xFF, 0xFF, 0xFF));
    /* Leading cap. */
    fill_rect(img, x + 2 + fill - 1, y + 1, 1, h - 2, C_AMBER_HI);
}

/* ------------------------------------------------------------------ app state */

typedef struct {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture  *texture;
    SDL_mutex    *lock;
    SDL_Thread   *worker;

    pit_image *canvas;

    log_buffer log;
    int step;
    int total_steps;
    double fraction;
    int busy;
    int done;
    int failed;
    pit_patch_result result;

    char input_path[512];
    char output_path[512];
    int  tab;            /* tab_id */
    int  hard_mode;      /* the optional data mod is on */
    int  editing;        /* 0 = none, 1 = input, 2 = output */
    int  focus;          /* FOCUS_* slot */
    ui_button buttons[BTN_COUNT];
    int button_count;
} ui_app;

static ui_app app;

/* The canvas is a pit_image the caller owns; pit_image_alloc fills it in place. */
static pit_image canvas_storage;

/*
 * Fills out_path with "<input>.hardmode.nds" while the mod is on, or
 * "<input>.prepared.nds" without it. Length is checked rather than relying on
 * snprintf to truncate, so a long path reports instead of silently producing a
 * name that collides with another ROM.
 */
static int set_default_output(char *out_path, size_t out_size, const char *input_path)
{
    size_t len = strlen(input_path);
    const char *suffix = app.hard_mode ? ".hardmode.nds" : ".prepared.nds";

    if (len + strlen(suffix) + 1 > out_size) {
        return 0;
    }
    memcpy(out_path, input_path, len);
    strcpy(out_path + len, suffix);
    return 1;
}

/* ------------------------------------------------------------------- logging */

static void log_add(log_level level, const char *text)
{
    log_buffer *log = &app.log;

    if (log->count < LOG_MAX) {
        log->line[log->count].level = level;
        snprintf(log->line[log->count].text, sizeof(log->line->text), "%s%s",
                 level_prefix(level), text);
        log->count++;
    } else {
        memmove(log->line, log->line + 1, sizeof(log->line[0]) * (LOG_MAX - 1));
        log->line[LOG_MAX - 1].level = level;
        snprintf(log->line[LOG_MAX - 1].text, sizeof(log->line[LOG_MAX - 1].text),
                 "%s%s", level_prefix(level), text);
    }
}

/* Called from the worker thread. */
static void on_patch_log(void *ctx, int step, const char *message, double fraction)
{
    (void)ctx;

    SDL_LockMutex(app.lock);
    if (step != app.step) {
        app.step = step;
    }
    app.fraction = fraction;
    log_add(fraction >= 1.0 ? LOG_OK : LOG_INFO, message);
    SDL_UnlockMutex(app.lock);
}

/* Called from the worker thread. SDL_CreateThread wants an int-returning fn. */
static int patch_worker(void *ctx)
{
    pit_patch_info info;
    pit_patch_result result;
    char text[128];

    (void)ctx;
    memset(&info, 0, sizeof(info));

    result = pit_patcher_run(app.input_path, app.output_path, app.hard_mode,
                             on_patch_log, NULL, &info);

    SDL_LockMutex(app.lock);
    app.result = result;
    app.failed = (result != PIT_PATCH_OK);
    app.busy = 0;
    app.done = 1;
    if (result == PIT_PATCH_OK) {
        if (info.records_patched > 0) {
            snprintf(text, sizeof(text), "Patched %u records, %u fields.",
                     info.records_patched, info.fields_written);
        } else {
            snprintf(text, sizeof(text), "Prepared ROM; no mods applied.");
        }
        log_add(LOG_OK, text);
    } else {
        log_add(LOG_ERROR, pit_patch_result_text(result));
    }
    app.fraction = 1.0;
    SDL_UnlockMutex(app.lock);
    return 0;
}

static void start_patch(void)
{
    if (app.busy) {
        return;
    }
    if (app.input_path[0] == '\0' || app.output_path[0] == '\0') {
        log_add(LOG_WARN, "Choose a source and an output ROM first.");
        return;
    }
    if (strcmp(app.input_path, app.output_path) == 0) {
        log_add(LOG_ERROR, "Output must differ from the source ROM.");
        return;
    }

    SDL_LockMutex(app.lock);
    app.log.count = 0;
    app.step = 0;
    app.fraction = 0.0;
    app.busy = 1;
    app.done = 0;
    app.failed = 0;
    SDL_UnlockMutex(app.lock);

    app.worker = SDL_CreateThread(patch_worker, "pit_patch", NULL);
    if (!app.worker) {
        SDL_LockMutex(app.lock);
        app.busy = 0;
        SDL_UnlockMutex(app.lock);
        log_add(LOG_ERROR, "Could not start the patch thread.");
    }
}

/* ------------------------------------------------------------- file dialogs */

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>

static int browse_file(char *out, size_t out_size, int save)
{
    /*
     * OPENFILENAMEA wants a double-NUL terminated filter list, so this is a
     * static array rather than a formatted string: a NUL inside a format
     * operand truncates the string that snprintf is meant to produce.
     */
    static const char filter[] =
        "Nintendo DS ROM (*.nds;*.dsi)\0*.nds;*.dsi\0"
        "All files (*.*)\0*.*\0\0";
    const char *title = save ? "Save patched ROM" : "Choose source ROM";
    OPENFILENAMEA ofn;

    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = filter;
    ofn.lpstrTitle = title;
    ofn.lpstrFile = out;
    ofn.nMaxFile = (DWORD)out_size;
    ofn.lpstrTitle = title;
    ofn.Flags = OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | (save ? 0 : OFN_FILEMUSTEXIST);

    return GetOpenFileNameA(&ofn) ? 1 : 0;
}
#else
static int browse_file(char *out, size_t out_size, int save)
{
    (void)out;
    (void)out_size;
    (void)save;
    return 0;
}
#endif

/* -------------------------------------------------------------------- render */

/* Backdrop: vertical gradient with a soft gold wash behind the title, so the
 * header has somewhere to glow from. */
static void draw_backdrop(pit_image *img)
{
    int y;
    int x;

    for (y = 0; y < UI_H; y++) {
        int t = (y * 255) / (UI_H - 1);
        pit_pixel c = lerp_color(C_BG_TOP, C_BG, t);

        fill_rect(img, 0, y, UI_W, 1, c);
    }
    for (y = 0; y < HEADER_H + 20; y++) {
        int fall = HEADER_H + 20 - y;
        pit_pixel c = PIT_ARGB(clampi(26 - fall, 0, 26) + 8, 0xDE, 0x94, 0x29);

        for (x = 0; x < UI_W; x++) {
            blend_pixel(img, x, y, c);
        }
    }
}

static void draw_header(pit_image *img)
{
    int i;

    /* Gold rule under the header, brightest in the middle. */
    for (i = 0; i < UI_W; i++) {
        int t = (i * 255) / (UI_W - 1);
        pit_pixel c = lerp_color(C_ACCENT_LO, C_AMBER_HI, t);

        put_pixel(img, i, HEADER_H - 2, c);
    }
    fill_rect(img, 0, HEADER_H - 1, UI_W, 1, C_EDGE_SOFT);

    /* Title with a 1px shadow. */
    draw_text(img, MARGIN + 1, 11 + 1, PIT_ARGB(160, 0x00, 0x00, 0x08),
              "PiT PATCHER", 3);
    draw_text(img, MARGIN, 10, C_TEXT_HI, "PiT PATCHER", 3);
    draw_text(img, MARGIN + 2, 30, C_ACCENT, "DECOMP ROM WORKSHOP", 1);

    /* Version chip, right aligned. */
    {
        const char *version = PIT_PLAN_VERSION;
        int chip_w = text_width(version, 1) + 14;
        int chip_x = UI_W - MARGIN - chip_w;

        fill_round_gradient(img, chip_x, 12, chip_w, 16, 3, C_PANEL_HI, C_PANEL_LO);
        stroke_round_rect(img, chip_x, 12, chip_w, 16, 3, C_EDGE_SOFT);
        draw_text(img, chip_x + 7, 16, C_AMBER, version, 1);
    }

    /* Region badge: a gold pill that says the ROM has to be EUR. */
    {
        const char *badge = "EUR ONLY";
        int pill_w = text_width(badge, 1) + 16;
        int pill_x = UI_W - MARGIN - pill_w;
        int pill_y = 12;

        if (pill_x - text_width(PIT_PLAN_VERSION, 1) - 20 > MARGIN) {
            pill_x -= text_width(PIT_PLAN_VERSION, 1) + 34;
            fill_round_gradient(img, pill_x, pill_y, pill_w, 16, 3,
                                C_AMBER, C_ACCENT);
            fill_rect(img, pill_x + 3, pill_y + 1, pill_w - 6, 1, C_AMBER_HI);
            stroke_round_rect(img, pill_x, pill_y, pill_w, 16, 3, C_ACCENT_LO);
            draw_text(img, pill_x + 8, pill_y + 4, C_PILL_TEXT, badge, 1);
        }
    }
}

/* One stat chip: a small card with the label and the multiplier. */
static void draw_chip(pit_image *img, int x, int y, int w, int h,
                      const char *label, const char *value, int hot)
{
    drop_shadow(img, x, y, w, h, 3);
    fill_round_gradient(img, x, y, w, h, 3, hot ? C_PANEL_HI : C_PANEL,
                        C_PANEL_LO);
    fill_rect(img, x + 4, y + 1, w - 8, 1, C_EDGE);
    stroke_round_rect(img, x, y, w, h, 3, C_EDGE_SOFT);

    /* Gold spine on the left edge marks it as a tuned stat. */
    fill_rect(img, x + 1, y + 4, 2, h - 8, hot ? C_AMBER : C_ACCENT);
    fill_rect(img, x + 1, y + 3, 2, 1, PIT_ARGB(160, 0xFF, 0xFF, 0xFF));

    draw_text(img, x + 8, y + 3, C_DIM, label, 1);
    draw_text(img, x + 8, y + 12, hot ? C_AMBER_HI : C_AMBER, value, 1);
}

static void draw_footer(pit_image *img)
{
    const char *tag = "(57%)";

    fill_rect(img, 0, FOOTER_Y - 2, UI_W, 1, C_EDGE_SOFT);
    draw_text(img, MARGIN, FOOTER_Y, C_DIM,
              "FOR THE PARTNERS IN TIME DECOMPILATION", 1);
    draw_text(img, UI_W - MARGIN - text_width(tag, 1), FOOTER_Y, C_AMBER,
              tag, 1);
}

static const char *field_label(int i)
{
    return (i == 0) ? "SOURCE" : "OUTPUT";
}

static void draw_fields(pit_image *img)
{
    int i;

    for (i = 0; i < 2; i++) {
        const char *value = (i == 0)
                                ? (app.input_path[0] ? app.input_path : "CHOOSE EUR ROM")
                                : (app.output_path[0] ? app.output_path : "PATCHED OUT");
        int editing = (app.editing == i + 1);
        int fy = FIELD_Y0 + i * FIELD_DY;

        draw_text(img, MARGIN, fy + 4, editing ? C_AMBER : C_DIM,
                  field_label(i), 1);
        draw_well(img, FIELD_X, fy, FIELD_W, FIELD_H, editing);
        if (!editing) {
            draw_text(img, FIELD_X + 4, fy + 4, C_TEXT, value, 1);
        } else {
            draw_text_clipped(img, FIELD_X + 4, fy + 4, C_TEXT_HI, value, 1,
                              FIELD_W - 8);
        }
        /* Caret while editing. */
        if (editing) {
            int caret_x = FIELD_X + 4 + text_width(value, 1) + 1;

            if (caret_x < FIELD_X + FIELD_W - 3) {
                fill_rect(img, caret_x, fy + 3, 1, FONT_H + 2, C_AMBER);
            }
        }
    }
}

/* PATCH tab: the fields, the plan status, the primary action and the log. */
static void draw_patch_tab(pit_image *img)
{
    char text[32];
    pit_pixel color;
    int i;

    draw_fields(img);

    /* Plan status on the left, keyboard hints on the right. */
    draw_text(img, MARGIN, MODS_Y,
              app.hard_mode ? C_AMBER : C_DIM,
              app.hard_mode ? "MODS: HARD MODE" : "MODS: NONE", 1);
    {
        const char *keys = "TAB FOCUS  ENTER PATCH  ESC QUIT";

        draw_text(img, UI_W - MARGIN - text_width(keys, 1), MODS_Y, C_DIM,
                  keys, 1);
    }

    for (i = 0; i < app.button_count; i++) {
        ui_button *btn = &app.buttons[i];

        if (btn->id == BTN_BROWSE_IN || btn->id == BTN_BROWSE_OUT ||
            btn->id == BTN_PATCH) {
            draw_button(img, btn);
        }
    }

    /* Status line. */
    {
        const char *caption = "STATUS";

        if (app.busy) {
            snprintf(text, sizeof(text), "WORKING %d/%d", app.step,
                     app.total_steps);
            color = C_AMBER;
        } else if (app.done && app.failed) {
            snprintf(text, sizeof(text), "FAILED");
            color = C_ERROR;
        } else if (app.done) {
            snprintf(text, sizeof(text), "DONE");
            color = C_OK;
        } else {
            snprintf(text, sizeof(text), "READY");
            color = C_DIM;
        }
        draw_text(img, STATUS_X, PATCH_Y + 4, C_DIM, caption, 1);
        draw_text(img, STATUS_X + text_width(caption, 1) + 8, PATCH_Y + 4, color,
                  text, 1);
    }
    draw_progress(img, BAR_X, BAR_Y, BAR_W, BAR_H, app.fraction);

    /* ----------------------------------------------------------------- log */
    draw_card(img, LOG_X, LOG_Y, LOG_W, LOG_H, 4);
    draw_text(img, LOG_X + 12, LOG_Y + 9, C_ACCENT, "ACTIVITY", 1);
    fill_rect(img, LOG_X + 12, LOG_Y + 20, LOG_W - 24, 1, C_EDGE_SOFT);

    {
        int rows = (LOG_H - 28) / LINE_H;
        int first = app.log.count - rows;
        int row;

        if (first < 0) {
            first = 0;
        }
        for (row = first; row < app.log.count; row++) {
            int ly = LOG_Y + 26 + (row - first) * LINE_H;
            int max_w = LOG_W - 24;
            pit_pixel color = level_color(app.log.line[row].level);

            /* Level dot, so severity reads without relying on the prefix. */
            if (app.log.line[row].level != LOG_INFO) {
                fill_round_rect(img, LOG_X + 12, ly + 2, 3, 3, 1, color);
            }
            draw_text_clipped(img, LOG_X + 20, ly, color,
                              app.log.line[row].text, 1, max_w);
        }
    }
}

/* MODS tab: the optional data mods. Each mod is a card with a toggle; when the
 * toggle is on the tuned stats show as chips, like the old plan card did. */
static void draw_mods_tab(pit_image *img)
{
    const char *state = app.hard_mode ? "ON" : "OFF";
    ui_button *toggle = &app.buttons[BTN_TOGGLE - BTN_TAB_PATCH];
    int i;

    draw_card(img, MODS_CARD_X, MODS_CARD_Y, MODS_CARD_W, MODS_CARD_H, 4);

    draw_text(img, MODS_CARD_X + 12, MODS_CARD_Y + 9, C_ACCENT, "MODS", 1);
    {
        const char *tag = "OPTIONAL";

        draw_text(img, MODS_CARD_X + MODS_CARD_W - 12 - text_width(tag, 1),
                  MODS_CARD_Y + 9, C_DIM, tag, 1);
    }
    fill_rect(img, MODS_CARD_X + 12, MODS_CARD_Y + 20, MODS_CARD_W - 24, 1,
              C_EDGE_SOFT);

    /* Toggle row. */
    draw_toggle(img, toggle, app.hard_mode);
    draw_text(img, MODS_NAME_X, MODS_NAME_Y, C_TEXT_HI, "HARD MODE", 1);
    {
        int sw = text_width(state, 1);

        draw_text(img, MODS_CARD_X + MODS_CARD_W - 12 - sw,
                  TOGGLE_Y + TOGGLE_H / 2 - 4,
                  app.hard_mode ? C_AMBER : C_DIM, state, 1);
    }

    draw_text(img, MODS_CARD_X + 12, MODS_DESC_Y, C_DIM,
              "A DATA MOD THAT RAISES ENEMY STATS: HP, POW, DEF", 1);
    draw_text(img, MODS_CARD_X + 12, MODS_DESC_Y + LINE_H, C_DIM,
              "AND SPD UP, PLUS 75% MORE EXPERIENCE AND COINS.", 1);

    fill_rect(img, MODS_CARD_X + 12, MODS_DIV_Y, MODS_CARD_W - 24, 1,
              C_EDGE_SOFT);

    if (app.hard_mode) {
        for (i = 0; i < (int)PIT_TRANSFORM_COUNT; i++) {
            const pit_plan_transform *t = &PIT_PLAN_TRANSFORMS[i];
            int col = i % CHIP_COUNT;
            int row = i / CHIP_COUNT;
            int cx = CHIP_X0 + col * (CHIP_W + CHIP_GAP);
            int cy = CHIP_Y0 + row * CHIP_DY;

            draw_chip(img, cx, cy, CHIP_W, CHIP_H, t->label, t->scale_text, 1);
        }
    } else {
        draw_text(img, MODS_CARD_X + 12, CHIP_Y0, C_DIM,
                  "OFF: THE PATCH ONLY VERIFIES AND PREPARES YOUR", 1);
        draw_text(img, MODS_CARD_X + 12, CHIP_Y0 + LINE_H, C_DIM,
                  "EUR COPY - NO MOD IS APPLIED. TAP THE TOGGLE TO", 1);
        draw_text(img, MODS_CARD_X + 12, CHIP_Y0 + 2 * LINE_H, C_DIM,
                  "ENABLE HARD MODE, THEN PATCH.", 1);
    }

    draw_text(img, MARGIN, MODS_HINT_Y, C_DIM,
              "MODS ARE OPTIONAL DATA EDITS TO YOUR OWN ROM COPY.", 1);
}

/* ABOUT tab: what this tool is for, and the honest state of the decompilation.
 * Every line stays under the card's text width (54 glyphs), so nothing clips. */
static void draw_about_tab(pit_image *img)
{
    static const char *lines[] = {
        "THIS PATCHER WORKS WITH YOUR OWN EUR COPY OF",
        "MARIO & LUIGI: PARTNERS IN TIME. IT VERIFIES THE",
        "ROM BY SHA-1, FINDS ITS STAT TABLE, AND WRITES A",
        "ROM FOR THE DECOMPILATION PROJECT.",
        "",
        "THE PROJECT IS ONLY ABOUT 57% RECONSTRUCTED, SO",
        "THE RESULT MAY NOT BOOT NATIVELY ON A CONSOLE OR",
        "EMULATOR YET - WE TAKE THE CHANCE ANYWAY.",
        "",
        "HARD MODE IS AN OPTIONAL DATA MOD (MODS TAB) THAT",
        "RAISES ENEMY STATS TO MAKE THE GAME HARDER. IT IS",
        "NOT REQUIRED TO USE THIS TOOL.",
    };
    int i;

    draw_card(img, ABOUT_CARD_X, ABOUT_CARD_Y, ABOUT_CARD_W, ABOUT_CARD_H, 4);

    draw_text(img, ABOUT_CARD_X + 12, ABOUT_CARD_Y + 9, C_ACCENT, "ABOUT", 1);
    {
        const char *tag = "PIT PATCHER";

        draw_text(img, ABOUT_CARD_X + ABOUT_CARD_W - 12 - text_width(tag, 1),
                  ABOUT_CARD_Y + 9, C_DIM, tag, 1);
    }
    fill_rect(img, ABOUT_CARD_X + 12, ABOUT_CARD_Y + 20, ABOUT_CARD_W - 24, 1,
              C_EDGE_SOFT);

    draw_text(img, ABOUT_CARD_X + 12, ABOUT_CARD_Y + 32, C_TEXT_HI,
              "PARTNERS IN TIME - DECOMPILATION EDITION", 1);
    fill_rect(img, ABOUT_CARD_X + 12, ABOUT_CARD_Y + 42, ABOUT_CARD_W - 24, 1,
              C_EDGE_SOFT);

    for (i = 0; i < (int)(sizeof(lines) / sizeof(lines[0])); i++) {
        int ly = ABOUT_CARD_Y + 54 + i * LINE_H;

        draw_text(img, ABOUT_CARD_X + 12, ly,
                  (lines[i][0] == '\0') ? C_DIM : C_TEXT, lines[i], 1);
    }

    {
        char info[80];

        snprintf(info, sizeof(info), "%s v%s | %d x %d-BYTE RECORDS",
                 PIT_PLAN_ID, PIT_PLAN_VERSION,
                 (int)PIT_RECORD_COUNT, (int)PIT_RECORD_SIZE);
        draw_text(img, ABOUT_CARD_X + 12,
                  ABOUT_CARD_Y + ABOUT_CARD_H - 14, C_DIM, info, 1);
    }
}

static void render(void)
{
    pit_image *img = app.canvas;
    int i;

    draw_backdrop(img);
    draw_header(img);

    /* Focus highlight for the controls before anything is drawn. */
    for (i = 0; i < app.button_count; i++) {
        ui_button *btn = &app.buttons[i];

        btn->focus_sel = 0;
        switch (btn->id) {
        case BTN_BROWSE_IN:  btn->focus_sel = (app.focus == FOCUS_BROWSE_IN); break;
        case BTN_BROWSE_OUT: btn->focus_sel = (app.focus == FOCUS_BROWSE_OUT); break;
        case BTN_PATCH:      btn->focus_sel = (app.focus == FOCUS_PATCH); break;
        case BTN_TOGGLE:     btn->focus_sel = (app.focus == FOCUS_TOGGLE); break;
        default:             break;
        }
    }

    /* Tabs. */
    for (i = 0; i < TAB_COUNT; i++) {
        ui_button *btn = &app.buttons[i];

        draw_tab(img, btn, app.tab == i);
    }

    if (app.tab == TAB_MODS) {
        draw_mods_tab(img);
    } else if (app.tab == TAB_ABOUT) {
        draw_about_tab(img);
    } else {
        draw_patch_tab(img);
    }

    draw_footer(img);
}

/* --------------------------------------------------------------------- input */

/* Button geometry is fixed at setup so render() never mutates the hit tests.
 * The first TAB_COUNT slots are the tabs, then the mods toggle, then the PATCH
 * tab's browse buttons and its primary action. */
static void setup_buttons(void)
{
    static const char *tab_label[TAB_COUNT] = { "PATCH", "MODS", "ABOUT" };
    int tab;
    int i = 0;

    memset(app.buttons, 0, sizeof(app.buttons));

    for (tab = 0; tab < TAB_COUNT; tab++) {
        app.buttons[i].id = (button_id)(BTN_TAB_PATCH + tab);
        app.buttons[i].x = TAB_X0 + tab * (TAB_W + TAB_GAP);
        app.buttons[i].y = TAB_Y;
        app.buttons[i].w = TAB_W;
        app.buttons[i].h = TAB_H;
        app.buttons[i].label = tab_label[tab];
        app.buttons[i].enabled = 1;
        i++;
    }

    app.buttons[i].id = BTN_TOGGLE;
    app.buttons[i].x = TOGGLE_X;
    app.buttons[i].y = TOGGLE_Y;
    app.buttons[i].w = TOGGLE_W;
    app.buttons[i].h = TOGGLE_H;
    app.buttons[i].label = "";
    app.buttons[i].enabled = 1;
    i++;

    app.buttons[i].id = BTN_BROWSE_IN;
    app.buttons[i].x = BROWSE_X;
    app.buttons[i].y = FIELD_Y0 - 2;
    app.buttons[i].w = BROWSE_W;
    app.buttons[i].h = BROWSE_H;
    app.buttons[i].label = "BROWSE";
    app.buttons[i].enabled = 1;
    i++;

    app.buttons[i].id = BTN_BROWSE_OUT;
    app.buttons[i].x = BROWSE_X;
    app.buttons[i].y = FIELD_Y0 + FIELD_DY - 2;
    app.buttons[i].w = BROWSE_W;
    app.buttons[i].h = BROWSE_H;
    app.buttons[i].label = "BROWSE";
    app.buttons[i].enabled = 1;
    i++;

    app.buttons[i].id = BTN_PATCH;
    app.buttons[i].x = PATCH_X;
    app.buttons[i].y = PATCH_Y;
    app.buttons[i].w = PATCH_W;
    app.buttons[i].h = PATCH_H;
    app.buttons[i].label = "PATCH";
    app.buttons[i].enabled = 1;
    i++;

    app.button_count = i;
}

/* Focus owns the editing state, so only set editing through here. */
static void set_focus(int focus)
{
    app.focus = focus;
    app.editing = (focus == FOCUS_SRC) ? 1 : (focus == FOCUS_OUT) ? 2 : 0;
}

static void switch_tab(int tab)
{
    if (tab < 0) {
        tab = TAB_COUNT - 1;
    }
    if (tab >= TAB_COUNT) {
        tab = 0;
    }
    app.tab = tab;
    set_focus(FOCUS_NONE);
}

static button_id focus_to_button(int focus)
{
    switch (focus) {
    case FOCUS_BROWSE_IN:  return BTN_BROWSE_IN;
    case FOCUS_BROWSE_OUT: return BTN_BROWSE_OUT;
    case FOCUS_PATCH:      return BTN_PATCH;
    case FOCUS_TOGGLE:     return BTN_TOGGLE;
    default:               return BTN_NONE;
    }
}

static void focus_cycle(int backward)
{
    static const int patch_cycle[] = {
        FOCUS_SRC, FOCUS_BROWSE_IN, FOCUS_OUT, FOCUS_BROWSE_OUT, FOCUS_PATCH
    };
    const int count = (int)(sizeof(patch_cycle) / sizeof(patch_cycle[0]));
    int idx = -1;
    int i;

    if (app.tab == TAB_MODS) {
        set_focus(FOCUS_TOGGLE);
        return;
    }
    if (app.tab == TAB_ABOUT) {
        set_focus(FOCUS_NONE);
        return;
    }
    for (i = 0; i < count; i++) {
        if (patch_cycle[i] == app.focus) {
            idx = i;
            break;
        }
    }
    if (idx < 0) {
        idx = backward ? count - 1 : 0;
    } else if (backward) {
        idx = (idx + count - 1) % count;
    } else {
        idx = (idx + 1) % count;
    }
    set_focus(patch_cycle[idx]);
}

static char *editing_buffer(void)
{
    if (app.editing == 1) {
        return app.input_path;
    }
    if (app.editing == 2) {
        return app.output_path;
    }
    return NULL;
}

static void activate(button_id id);   /* defined after the key handling */

static void handle_text_input(const char *text)
{
    char *buffer = editing_buffer();
    size_t len;

    if (!buffer || !*text) {
        return;
    }
    len = strlen(buffer);
    while (*text) {
        if (len + 1 < sizeof(app.input_path)) {
            buffer[len++] = *text;
            buffer[len] = '\0';
        }
        text++;
    }
}

static void handle_key(SDL_Keycode key, Uint16 mod)
{
    char *buffer = editing_buffer();
    size_t len;

    switch (key) {
    case SDLK_ESCAPE:
        if (buffer) {
            set_focus(FOCUS_NONE);
        } else if (app.busy) {
            log_add(LOG_WARN, "Still working; wait for the current step.");
        } else {
            app.done = 2;
        }
        break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        if (buffer) {
            set_focus(FOCUS_NONE);
        } else {
            button_id id = focus_to_button(app.focus);

            if (id != BTN_NONE) {
                activate(id);
            } else if (app.tab == TAB_PATCH) {
                start_patch();
            }
        }
        break;
    case SDLK_SPACE:
        if (!buffer) {
            button_id id = focus_to_button(app.focus);

            if (id != BTN_NONE) {
                activate(id);
            }
        }
        break;
    case SDLK_TAB: {
        int backward = (mod & KMOD_SHIFT) != 0;

        focus_cycle(backward);
        break;
    }
    case SDLK_LEFT:
    case SDLK_RIGHT:
        if (!buffer) {
            switch_tab(app.tab + (key == SDLK_RIGHT ? 1 : -1));
        }
        break;
    case SDLK_BACKSPACE:
        if (buffer) {
            len = strlen(buffer);
            if (len > 0) {
                buffer[len - 1] = '\0';
            }
        }
        break;
    case SDLK_s:
        if (!buffer && !app.busy && app.tab == TAB_PATCH) {
            if (app.input_path[0] && app.output_path[0] == '\0') {
                if (set_default_output(app.output_path, sizeof(app.output_path),
                                       app.input_path)) {
                    log_add(LOG_INFO, "Output set alongside the source.");
                } else {
                    log_add(LOG_ERROR, "Source path is too long to derive an output.");
                }
            }
        }
        break;
    default:
        break;
    }
}

static void handle_mouse_motion(int mx, int my)
{
    int i;

    for (i = 0; i < app.button_count; i++) {
        ui_button *btn = &app.buttons[i];

        btn->hovered = (mx >= btn->x && mx < btn->x + btn->w &&
                        my >= btn->y && my < btn->y + btn->h);
        if (!btn->hovered) {
            btn->pressed = 0;
        }
    }
}

/* A click that lands in an editable well puts the keyboard focus there. */
static int field_hit(int mx, int my, int *field)
{
    int i;

    for (i = 0; i < 2; i++) {
        int fy = FIELD_Y0 + i * FIELD_DY;

        if (mx >= FIELD_X && mx < FIELD_X + FIELD_W &&
            my >= fy && my < fy + FIELD_H) {
            *field = (i == 0) ? FOCUS_SRC : FOCUS_OUT;
            return 1;
        }
    }
    return 0;
}

static void activate(button_id id)
{
    switch (id) {
    case BTN_TAB_PATCH:
    case BTN_TAB_MODS:
    case BTN_TAB_ABOUT:
        switch_tab(id - (int)BTN_TAB_PATCH);
        break;
    case BTN_TOGGLE:
        if (app.tab == TAB_MODS) {
            app.hard_mode = !app.hard_mode;
            set_focus(FOCUS_TOGGLE);
            log_add(LOG_INFO, app.hard_mode ? "Hard Mode ON: enemy stats are tuned."
                                            : "Hard Mode OFF: prepare only.");
        }
        break;
    case BTN_BROWSE_IN:
        if (app.tab != TAB_PATCH || app.busy) {
            break;
        }
        if (browse_file(app.input_path, sizeof(app.input_path), 0)) {
            if (app.output_path[0] == '\0') {
                set_default_output(app.output_path, sizeof(app.output_path),
                                   app.input_path);
            }
            log_add(LOG_INFO, "Source ROM selected.");
        }
        break;
    case BTN_BROWSE_OUT:
        if (app.tab != TAB_PATCH || app.busy) {
            break;
        }
        if (browse_file(app.output_path, sizeof(app.output_path), 1)) {
            log_add(LOG_INFO, "Output path selected.");
        }
        break;
    case BTN_PATCH:
        if (app.tab == TAB_PATCH) {
            start_patch();
        }
        break;
    default:
        break;
    }
}

/* ---------------------------------------------------------------------- main */

/*
 * Maps a canvas pixel to one character so the layout can be checked from a
 * terminal or a build log, where an image cannot be inspected. This is the same
 * render() output the window shows, just serialised as text.
 *
 * The UI uses gradients, so an exact palette match would only match a few
 * pixels. Pixels are bucketed by channel relationships instead: warm hues become
 * gold characters, neutral blues by brightness, and anything bright and
 * desaturated is text.
 */
static char ascii_class(pit_pixel c)
{
    int r = (int)((c & 0x00FF0000u) >> 16);
    int g = (int)((c & 0x0000FF00u) >> 8);
    int b = (int)(c & 0x000000FFu);
    int lum = (r * 30 + g * 59 + b * 11) / 100;
    int warm = r - b;

    if (warm > 40) {
        /* Gold family: accents, the primary button, the progress fill. */
        return (lum > 170) ? 'M' : 'A';
    }
    if (r > 180 && g > 120 && b < 120) {
        return 'X';
    }
    if (g > 150 && b < 140 && r < 150) {
        return 'O';
    }
    if (lum < 12)  { return ' '; }  /* backdrop */
    if (lum < 40)  { return '.'; }  /* panel body */
    if (lum < 90)  { return ':'; }  /* dim text, borders */
    if (lum < 150) { return '+'; }  /* pale text */
    return '#';                     /* highlights */
}

static void dump_ascii(int step_x, int step_y)
{
    int y;
    int x;

    for (y = 0; y < UI_H; y += step_y) {
        for (x = 0; x < UI_W; x += step_x) {
            putchar(ascii_class(pit_image_get(app.canvas, x, y)));
        }
        putchar('\n');
    }
}

static int parse_tab_arg(const char *text)
{
    if (strcmp(text, "MODS") == 0) {
        return TAB_MODS;
    }
    if (strcmp(text, "ABOUT") == 0) {
        return TAB_ABOUT;
    }
    if (strcmp(text, "PATCH") == 0) {
        return TAB_PATCH;
    }
    return atoi(text);
}

static void usage_flags(const char *argv0)
{
    fprintf(stderr,
            "usage: %s [--screenshot out.png|--dump-ascii] [--tab PATCH|MODS|ABOUT]\n"
            "            [--mods on|off] [--hover N] [--simulate] [input.nds [output.nds]]\n",
            argv0);
}

int pit_patcher_ui_main(int argc, char **argv)
{
    SDL_Event event;
    int running = 1;
    int mouse_x = 0;
    int mouse_y = 0;
    unsigned char *rgba = (unsigned char *)malloc((size_t)UI_W * UI_H * 4);
    const char *screenshot_path = NULL;
    int simulate = 0;
    int dump = 0;
    int hover = -1;
    int positional = 0;
    int i;

    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];

        if (strcmp(arg, "--screenshot") == 0 && i + 1 < argc) {
            screenshot_path = argv[++i];
        } else if (strcmp(arg, "--dump-ascii") == 0) {
            screenshot_path = "-";
            dump = 1;
        } else if (strcmp(arg, "--simulate") == 0) {
            simulate = 1;
        } else if (strcmp(arg, "--tab") == 0 && i + 1 < argc) {
            app.tab = parse_tab_arg(argv[++i]);
        } else if (strcmp(arg, "--mods") == 0 && i + 1 < argc) {
            const char *flag = argv[++i];

            app.hard_mode = (strcmp(flag, "on") == 0 || strcmp(flag, "1") == 0);
        } else if (strcmp(arg, "--hover") == 0 && i + 1 < argc) {
            hover = atoi(argv[++i]);
        } else if (arg[0] == '-') {
            usage_flags(argv[0]);
            free(rgba);
            return 2;
        } else if (positional == 0) {
            snprintf(app.input_path, sizeof(app.input_path), "%s", arg);
            positional++;
        } else if (positional == 1) {
            snprintf(app.output_path, sizeof(app.output_path), "%s", arg);
            positional++;
        } else {
            usage_flags(argv[0]);
            free(rgba);
            return 2;
        }
    }

    if (!rgba) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    app.total_steps = PIT_PATCH_STEPS;
    setup_buttons();
    /* Seeded here rather than in render(), so the headless screenshot and the
     * window show the same log. */
    log_add(LOG_INFO, "Select a supported EUR ROM to begin.");
    log_add(LOG_INFO, "Tab moves focus, Enter activates, Esc quits.");
    log_add(LOG_INFO, "Hard Mode is optional: open the MODS tab.");

    if (pit_image_alloc(&canvas_storage, UI_W, UI_H) != 0) {
        fprintf(stderr, "could not allocate the %dx%d canvas\n", UI_W, UI_H);
        free(rgba);
        return 1;
    }
    app.canvas = &canvas_storage;

    if (screenshot_path) {
        /*
         * Headless render, used to check the layout without a display. The
         * frame is drawn through exactly the same render() the window uses, so
         * a screenshot cannot pass while the on-screen UI is broken.
         */
        int rc = 0;

        if (simulate) {
            app.busy = 1;
            app.step = 6;
            app.fraction = 0.5;
            log_add(LOG_INFO, "SHA-1 matches the supported EUR release.");
            log_add(LOG_INFO, "Title MARIO&LUIGI2 (ARMP) confirmed.");
            log_add(LOG_INFO, "Header CRC-16 D0BC verified.");
            log_add(LOG_INFO, "Found BData/BDataMon.dat at 0x00340000.");
            log_add(LOG_INFO, "Patched record 49/98...");
            log_add(LOG_WARN, "Output directory is not writable; check permissions.");
        }
        if (hover >= 0 && hover < app.button_count) {
            app.buttons[hover].hovered = 1;
        }
        render();
        if (dump) {
            dump_ascii(2, 4);
        }
        if (!dump) {
            if (pit_png_write(screenshot_path, app.canvas) != 0) {
                fprintf(stderr, "could not write %s\n", screenshot_path);
                rc = 1;
            } else {
                printf("wrote %s (%dx%d)\n", screenshot_path, UI_W, UI_H);
            }
        }
        pit_image_free(&canvas_storage);
        free(rgba);
        return rc;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        pit_image_free(&canvas_storage);
        return 1;
    }

    app.window = SDL_CreateWindow("PiT Patcher - Decomp",
                                  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  UI_W * 2, UI_H * 2, SDL_WINDOW_RESIZABLE);
    app.renderer = app.window ? SDL_CreateRenderer(app.window, -1, 0) : NULL;
    if (!app.window || !app.renderer) {
        fprintf(stderr, "could not create a window: %s\n", SDL_GetError());
        SDL_Quit();
        pit_image_free(&canvas_storage);
        return 1;
    }

    /* pit_image_to_rgba8 writes R,G,B,A bytes, so the texture format must be
     * ABGR8888, whose in-memory order is R,G,B,A. ARGB8888 is stored as
     * B,G,R,A and would swap the red and blue channels. */
    app.texture = SDL_CreateTexture(app.renderer, SDL_PIXELFORMAT_ABGR8888,
                                    SDL_TEXTUREACCESS_STREAMING, UI_W, UI_H);
    if (!app.texture) {
        fprintf(stderr, "could not create the texture: %s\n", SDL_GetError());
        SDL_Quit();
        pit_image_free(&canvas_storage);
        return 1;
    }
    SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
    SDL_RenderSetLogicalSize(app.renderer, UI_W, UI_H);
    SDL_RenderSetVSync(app.renderer, 1);

    app.lock = SDL_CreateMutex();
    if (!app.lock) {
        fprintf(stderr, "could not create the mutex: %s\n", SDL_GetError());
        SDL_Quit();
        pit_image_free(&canvas_storage);
        return 1;
    }

    while (running && app.done != 2) {
        SDL_LockMutex(app.lock);
        render();
        SDL_UnlockMutex(app.lock);

        if (pit_image_to_rgba8(app.canvas, rgba) == 0) {
            SDL_UpdateTexture(app.texture, NULL, rgba, UI_W * 4);
        }
        SDL_RenderClear(app.renderer);
        SDL_RenderCopy(app.renderer, app.texture, NULL, NULL);
        SDL_RenderPresent(app.renderer);

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = 0;
                break;
            case SDL_KEYDOWN:
                handle_key(event.key.keysym.sym, event.key.keysym.mod);
                break;
            case SDL_TEXTINPUT:
                handle_text_input(event.text.text);
                break;
            case SDL_MOUSEMOTION: {
                int w = 1, h = 1;

                SDL_GetRendererOutputSize(app.renderer, &w, &h);
                mouse_x = event.motion.x * UI_W / (w ? w : 1);
                mouse_y = event.motion.y * UI_H / (h ? h : 1);
                handle_mouse_motion(mouse_x, mouse_y);
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    int field = FOCUS_NONE;

                    handle_mouse_motion(mouse_x, mouse_y);
                    if (app.tab == TAB_PATCH && field_hit(mouse_x, mouse_y, &field)) {
                        set_focus(field);
                    } else {
                        for (i = 0; i < app.button_count; i++) {
                            if (app.buttons[i].hovered && app.buttons[i].enabled) {
                                app.buttons[i].pressed = 1;
                                if (app.buttons[i].id == BTN_BROWSE_IN ||
                                    app.buttons[i].id == BTN_BROWSE_OUT ||
                                    app.buttons[i].id == BTN_PATCH ||
                                    app.buttons[i].id == BTN_TOGGLE) {
                                    set_focus(app.buttons[i].id == BTN_BROWSE_IN
                                                  ? FOCUS_BROWSE_IN
                                                  : app.buttons[i].id == BTN_BROWSE_OUT
                                                  ? FOCUS_BROWSE_OUT
                                                  : app.buttons[i].id == BTN_PATCH
                                                  ? FOCUS_PATCH
                                                  : FOCUS_TOGGLE);
                                }
                            }
                        }
                    }
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    handle_mouse_motion(mouse_x, mouse_y);
                    for (i = 0; i < app.button_count; i++) {
                        if (app.buttons[i].pressed && app.buttons[i].hovered) {
                            activate(app.buttons[i].id);
                        }
                        app.buttons[i].pressed = 0;
                    }
                }
                break;
            default:
                break;
            }
        }
        SDL_Delay(16);
    }

    if (app.worker) {
        SDL_WaitThread(app.worker, NULL);
    }
    SDL_DestroyMutex(app.lock);
    SDL_DestroyTexture(app.texture);
    SDL_DestroyRenderer(app.renderer);
    SDL_DestroyWindow(app.window);
    SDL_Quit();
    free(rgba);
    pit_image_free(&canvas_storage);
    return app.failed ? 1 : 0;
}