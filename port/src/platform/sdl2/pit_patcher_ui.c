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
#define C_PANEL     PIT_ARGB(255, 0x00, 0x10, 0x6B)  /* ROM 0x01C7420[16] */
#define C_PANEL_ALT PIT_ARGB(255, 0x00, 0x10, 0x94)  /* ROM 0x01C7420[18] */
#define C_EDGE      PIT_ARGB(255, 0x52, 0x63, 0x7B)  /* ROM 0x01C7420[8]  blue grey */
#define C_EDGE_HOT  PIT_ARGB(255, 0xFF, 0xA5, 0x21)  /* ROM 0x04D12FC bright gold */
#define C_TEXT      PIT_ARGB(255, 0xC5, 0xDE, 0xF7)  /* ROM 0x01C7420[15] pale blue */
#define C_DIM       PIT_ARGB(255, 0x73, 0x84, 0x94)  /* ROM 0x01C7420[9]  */
#define C_ACCENT    PIT_ARGB(255, 0xDE, 0x94, 0x29)  /* ROM 0x13661E8[60] gold */
#define C_AMBER     PIT_ARGB(255, 0xFF, 0x94, 0x21)  /* ROM 0x04D12FC[2]  bright gold */
#define C_ERROR     PIT_ARGB(255, 0xEF, 0x5A, 0x63)  /* ROM 0x03BFA00 soft red */
#define C_OK        PIT_ARGB(255, 0x52, 0xC5, 0x5A)  /* ROM 0x0810934 green */
#define C_SHADOW    PIT_ARGB(160, 0x00, 0x00, 0x08)  /* ROM 0x01C7420[21] */

#define UI_W 400
#define UI_H 240
#define FONT_W 8
#define FONT_H 8
#define LINE_H 10

/* --------------------------------------------------------------------- logs */

#define LOG_MAX 64

typedef enum {
    LOG_INFO = 0,
    LOG_GOOD,
    LOG_WARN,
    LOG_BAD
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
    BTN_NONE = 0,
    BTN_BROWSE_IN,
    BTN_BROWSE_OUT,
    BTN_PATCH
} button_id;

static const char *level_prefix(log_level level)
{
    switch (level) {
    case LOG_GOOD: return "+ ";
    case LOG_WARN: return "! ";
    case LOG_BAD:  return "X ";
    default:       return "  ";
    }
}

static pit_pixel level_color(log_level level)
{
    switch (level) {
    case LOG_GOOD: return C_OK;
    case LOG_WARN: return C_AMBER;
    case LOG_BAD:  return C_ERROR;
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

static void draw_panel(pit_image *img, int x, int y, int w, int h)
{
    fill_rect(img, x + 1, y + 1, w, h, C_SHADOW);
    fill_rect(img, x, y, w, h, C_PANEL);
    /* 1px inner highlight on the top edge gives the panels a bevel. */
    fill_rect(img, x, y, w, 1, C_EDGE);
    fill_rect(img, x, y + h - 1, w, 1, C_EDGE);
    fill_rect(img, x, y, 1, h, C_EDGE);
    fill_rect(img, x + w - 1, y, 1, h, C_EDGE);
}

typedef struct {
    button_id id;
    int x, y, w, h;
    const char *label;
    int enabled;
    int hovered;
    int pressed;
} ui_button;

static void draw_button(pit_image *img, ui_button *btn)
{
    pit_pixel face = btn->enabled ? C_PANEL_ALT : C_PANEL;
    pit_pixel edge = C_EDGE;
    int text_color = btn->enabled ? C_TEXT : C_DIM;

    if (btn->enabled && btn->hovered) {
        face = btn->pressed ? C_EDGE : C_EDGE_HOT;
        text_color = btn->pressed ? C_BG : C_BG;
        edge = C_EDGE_HOT;
    }
    if (btn->enabled && btn->id == BTN_PATCH) {
        edge = btn->hovered ? C_EDGE_HOT : C_ACCENT;
        text_color = btn->enabled ? (btn->hovered ? C_BG : C_ACCENT) : C_DIM;
    }

    fill_rect(img, btn->x + 1, btn->y + 1, btn->w, btn->h, C_SHADOW);
    fill_rect(img, btn->x, btn->y, btn->w, btn->h, face);
    fill_rect(img, btn->x, btn->y, btn->w, 1, edge);
    fill_rect(img, btn->x, btn->y + btn->h - 1, btn->w, 1, edge);
    fill_rect(img, btn->x, btn->y, 1, btn->h, edge);
    fill_rect(img, btn->x + btn->w - 1, btn->y, 1, btn->h, edge);

    {
        int tx = btn->x + (btn->w - text_width(btn->label, 1)) / 2;
        int ty = btn->y + (btn->h - FONT_H) / 2;

        draw_text(img, tx, ty, text_color, btn->label, 1);
    }
}

static void draw_progress(pit_image *img, int x, int y, int w, int h,
                          double fraction)
{
    int fill;

    if (fraction < 0.0) {
        fraction = 0.0;
    }
    if (fraction > 1.0) {
        fraction = 1.0;
    }
    fill_rect(img, x, y, w, h, PIT_ARGB(255, 0x08, 0x0B, 0x14));
    fill_rect(img, x, y, w, 1, C_EDGE);
    fill_rect(img, x, y + h - 1, w, 1, C_EDGE);
    fill_rect(img, x, y, 1, h, C_EDGE);
    fill_rect(img, x + w - 1, y, 1, h, C_EDGE);

    fill = (int)((double)(w - 4) * fraction);
    if (fill > 0) {
        int row;

        for (row = 0; row < h - 2; row++) {
            /* 1px gaps every other row read as a segmented bar. */
            pit_pixel c = (row % 2 == 0) ? C_ACCENT : C_AMBER;

            fill_rect(img, x + 2, y + 1 + row, fill, 1, c);
        }
    }
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
    int  editing;          /* 0 = none, 1 = input, 2 = output */
    ui_button buttons[3];
    int button_count;
} ui_app;

static ui_app app;

/* The canvas is a pit_image the caller owns; pit_image_alloc fills it in place. */
static pit_image canvas_storage;

/*
 * Fills out_path with "<input>.hardmode.nds". Length is checked rather than
 * relying on snprintf to truncate, so a long path reports instead of silently
 * producing a name that collides with another ROM.
 */
static int set_default_output(char *out_path, size_t out_size, const char *input_path)
{
    size_t len = strlen(input_path);
    static const char suffix[] = ".hardmode.nds";

    if (len + sizeof(suffix) > out_size) {
        return 0;
    }
    memcpy(out_path, input_path, len);
    memcpy(out_path + len, suffix, sizeof(suffix));
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
    log_add(fraction >= 1.0 ? LOG_GOOD : LOG_INFO, message);
    SDL_UnlockMutex(app.lock);
}

/* Called from the worker thread. SDL_CreateThread wants an int-returning fn. */
static int patch_worker(void *ctx)
{
    pit_patch_info info;
    pit_patch_result result;

    (void)ctx;
    memset(&info, 0, sizeof(info));

    result = pit_patcher_run(app.input_path, app.output_path,
                             on_patch_log, NULL, &info);

    SDL_LockMutex(app.lock);
    app.result = result;
    app.failed = (result != PIT_PATCH_OK);
    app.busy = 0;
    app.done = 1;
    if (result == PIT_PATCH_OK) {
        char text[128];

        snprintf(text, sizeof(text), "Patched %u records, %u fields.",
                 info.records_patched, info.fields_written);
        log_add(LOG_GOOD, text);
    } else {
        log_add(LOG_BAD, pit_patch_result_text(result));
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
        log_add(LOG_BAD, "Output must differ from the source ROM.");
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
        log_add(LOG_BAD, "Could not start the patch thread.");
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

static void render(void)
{
    pit_image *img = app.canvas;
    int i;
    int y;

    pit_image_fill(img, C_BG);

    /* Title bar. */
    draw_text(img, 8, 6, C_ACCENT, "PiT PATCHER", 2);
    draw_text(img, 8 + text_width("PiT PATCHER", 2) + 8, 12, C_DIM,
              PIT_PLAN_VERSION, 1);
    draw_text(img, UI_W - 8 - text_width("EUR ONLY", 1), 8, C_AMBER, "EUR ONLY", 1);
    fill_rect(img, 8, 26, UI_W - 16, 1, C_EDGE);

    /* Plan panel. */
    draw_panel(img, 8, 32, UI_W - 16, 62);
    draw_text(img, 14, 36, C_ACCENT, "PLAN", 1);
    draw_text(img, 14 + 40, 36, C_TEXT, PIT_PLAN_NAME, 1);
    draw_text(img, UI_W - 14 - text_width("98 RECORDS", 1), 36, C_DIM,
              "98 RECORDS", 1);

    for (i = 0; i < (int)PIT_TRANSFORM_COUNT; i++) {
        const pit_plan_transform *t = &PIT_PLAN_TRANSFORMS[i];
        int col = (i % 2) * ((UI_W - 16) / 2);
        int row = i / 2;
        int tx = 14 + col;
        int ty = 50 + row * LINE_H;
        char scale_text[24];

        if (t->den == 1u) {
            snprintf(scale_text, sizeof(scale_text), "x%d", (int)t->num);
        } else {
            snprintf(scale_text, sizeof(scale_text), "x%d/%d",
                     (int)t->num, (int)t->den);
        }
        draw_text(img, tx, ty, C_TEXT, t->label, 1);
        draw_text(img, tx + 96, ty, C_AMBER, scale_text, 1);
    }
    fill_rect(img, 8, 98, UI_W - 16, 1, C_EDGE);

    /* Path rows. */
    y = 104;
    draw_text(img, 8, y, C_DIM, "SOURCE", 1);
    fill_rect(img, 56, y - 1, UI_W - 56 - 8 - 74, FONT_H + 2,
              app.editing == 1 ? C_EDGE_HOT : C_PANEL);
    draw_text_clipped(img, 60, y, C_TEXT, app.input_path[0] ? app.input_path : "(none)",
                      1, UI_W - 56 - 8 - 74 - 8);

    y += 16;
    draw_text(img, 8, y, C_DIM, "OUTPUT", 1);
    fill_rect(img, 56, y - 1, UI_W - 56 - 8 - 74, FONT_H + 2,
              app.editing == 2 ? C_EDGE_HOT : C_PANEL);
    draw_text_clipped(img, 60, y, C_TEXT, app.output_path[0] ? app.output_path : "(none)",
                      1, UI_W - 56 - 8 - 74 - 8);

    for (i = 0; i < app.button_count; i++) {
        draw_button(img, &app.buttons[i]);
    }

    /* Progress. */
    y = 152;
    draw_text(img, 8, y, C_DIM, "STEPS", 1);
    {
        char text[32];

        if (app.step <= 0) {
            snprintf(text, sizeof(text), "IDLE");
        } else if (app.done) {
            snprintf(text, sizeof(text), "%s", app.failed ? "FAILED" : "DONE");
        } else {
            snprintf(text, sizeof(text), "%d/%d", app.step, app.total_steps);
        }
        draw_text(img, 44, y,
                  app.done ? (app.failed ? C_ERROR : C_OK) : C_TEXT, text, 1);
    }
    draw_progress(img, 100, y - 1, UI_W - 100 - 8, FONT_H + 2, app.fraction);

    /* Log pane. */
    y = 168;
    draw_panel(img, 8, y, UI_W - 16, UI_H - y - 8);
    {
        int rows = (UI_H - y - 10) / LINE_H;
        int first = app.log.count - rows;
        int row;

        if (first < 0) {
            first = 0;
        }
        for (row = first; row < app.log.count; row++) {
            int ly = y + 4 + (row - first) * LINE_H;
            int max_w = UI_W - 16 - 8;

            draw_text_clipped(img, 12, ly, level_color(app.log.line[row].level),
                              app.log.line[row].text, 1, max_w);
        }
    }
}

/* --------------------------------------------------------------------- input */

/* Button geometry is fixed at setup so render() never mutates the hit tests. */
static void setup_buttons(void)
{
    app.buttons[0].id = BTN_BROWSE_IN;
    app.buttons[0].x = UI_W - 8 - 70;
    app.buttons[0].y = 102;
    app.buttons[0].w = 70;
    app.buttons[0].h = 12;
    app.buttons[0].label = "BROWSE";
    app.buttons[0].enabled = 1;
    app.buttons[1] = app.buttons[0];
    app.buttons[1].id = BTN_BROWSE_OUT;
    app.buttons[1].y = 118;
    app.buttons[2] = app.buttons[0];
    app.buttons[2].id = BTN_PATCH;
    app.buttons[2].y = 134;
    app.buttons[2].label = "PATCH ROM";
    app.button_count = 3;
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

static void handle_key(SDL_Keycode key)
{
    char *buffer = editing_buffer();
    size_t len;

    switch (key) {
    case SDLK_ESCAPE:
        if (app.busy) {
            log_add(LOG_WARN, "Still working; wait for the current step.");
        } else {
            app.done = 2;
        }
        break;
    case SDLK_RETURN:
        if (buffer) {
            app.editing = 0;
        } else {
            start_patch();
        }
        break;
    case SDLK_TAB:
        app.editing = (app.editing == 1) ? 2 : (app.editing == 2 ? 0 : 1);
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
        if (!buffer && !app.busy) {
            if (app.input_path[0] && app.output_path[0] == '\0') {
                if (set_default_output(app.output_path, sizeof(app.output_path),
                                       app.input_path)) {
                    log_add(LOG_INFO, "Output set alongside the source.");
                } else {
                    log_add(LOG_BAD, "Source path is too long to derive an output.");
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

static void activate(button_id id)
{
    switch (id) {
    case BTN_BROWSE_IN:
        if (app.busy) {
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
        if (!app.busy && browse_file(app.output_path, sizeof(app.output_path), 1)) {
            log_add(LOG_INFO, "Output path selected.");
        }
        break;
    case BTN_PATCH:
        start_patch();
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
 */
static char ascii_class(pit_pixel c)
{
    if (c == C_BG)        { return ' '; }
    if (c == C_PANEL)     { return '.'; }
    if (c == C_PANEL_ALT) { return '='; }
    if (c == C_EDGE)      { return '-'; }
    if (c == C_EDGE_HOT)  { return '#'; }
    if (c == C_ACCENT)    { return 'A'; }
    if (c == C_AMBER)     { return 'M'; }
    if (c == C_TEXT)      { return '#'; }
    if (c == C_DIM)       { return ':'; }
    if (c == C_ERROR)     { return 'X'; }
    if (c == C_OK)        { return 'O'; }
    return '?';
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
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) {
            screenshot_path = argv[++i];
        } else if (strcmp(argv[i], "--dump-ascii") == 0) {
            screenshot_path = "-";
            dump = 1;
        } else if (strcmp(argv[i], "--simulate") == 0) {
            simulate = 1;
        }
    }

    if (!rgba) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    memset(&app, 0, sizeof(app));
    app.total_steps = PIT_PATCH_STEPS;
    setup_buttons();

    if (screenshot_path && argc > 1) {
        snprintf(app.input_path, sizeof(app.input_path), "%s", argv[1]);
    }
    if (screenshot_path && argc > 2) {
        snprintf(app.output_path, sizeof(app.output_path), "%s", argv[2]);
    }
    if (argc > 1 && !screenshot_path) {
        snprintf(app.input_path, sizeof(app.input_path), "%s", argv[1]);
    }
    if (argc > 2 && !screenshot_path) {
        snprintf(app.output_path, sizeof(app.output_path), "%s", argv[2]);
    }

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
        }
        render();
        if (dump) {
            dump_ascii(2, 4);
        }
        if (screenshot_path && !dump) {
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

    app.window = SDL_CreateWindow("PiT Patcher",
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

    log_add(LOG_INFO, "Select a supported EUR ROM to begin.");
    log_add(LOG_INFO, "Tab switches fields, Enter patches, Esc quits.");

    while (running && app.done != 2) {
        int mouse_down = 0;

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
                handle_key(event.key.keysym.sym);
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
                    mouse_down = 1;
                    handle_mouse_motion(mouse_x, mouse_y);
                    for (i = 0; i < app.button_count; i++) {
                        if (app.buttons[i].hovered && app.buttons[i].enabled) {
                            app.buttons[i].pressed = 1;
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
        (void)mouse_down;
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
