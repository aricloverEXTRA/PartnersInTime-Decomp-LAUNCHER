#include "platform/pit_platform.h"

#include "core/pit_video.h"

#include <SDL.h>

#define PIT_DEFAULT_SCALE 3

static SDL_Window   *g_window;
static SDL_Renderer *g_renderer;
static SDL_Texture  *g_texture[PIT_SCREEN_COUNT];
static int           g_quit;
static int           g_scale = PIT_DEFAULT_SCALE;

static void compute_window_size(pit_layout_mode layout, int scale, int *w, int *h)
{
    switch (layout) {
    case PIT_LAYOUT_SIDE_BY_SIDE:
        *w = PIT_SCREEN_WIDTH * 2 * scale;
        *h = PIT_SCREEN_HEIGHT * scale;
        break;
    case PIT_LAYOUT_OVERLAY:
        *w = PIT_SCREEN_WIDTH * scale;
        *h = PIT_SCREEN_HEIGHT * scale;
        break;
    case PIT_LAYOUT_STACKED:
    default:
        *w = PIT_SCREEN_WIDTH * scale;
        *h = PIT_SCREEN_HEIGHT * 2 * scale;
        break;
    }
}

static SDL_Rect screen_rect(int index, pit_layout_mode layout, int scale)
{
    SDL_Rect rect;

    rect.w = PIT_SCREEN_WIDTH * scale;
    rect.h = PIT_SCREEN_HEIGHT * scale;

    switch (layout) {
    case PIT_LAYOUT_SIDE_BY_SIDE:
        rect.x = index * rect.w;
        rect.y = 0;
        break;
    case PIT_LAYOUT_OVERLAY:
        rect.x = 0;
        rect.y = 0;
        break;
    case PIT_LAYOUT_STACKED:
    default:
        rect.x = 0;
        rect.y = index * rect.h;
        break;
    }

    return rect;
}

int pit_platform_video_init(const char *title)
{
    int i;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return -1;
    }

    g_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        PIT_SCREEN_WIDTH * g_scale,
        PIT_SCREEN_HEIGHT * 2 * g_scale,
        SDL_WINDOW_RESIZABLE);
    if (g_window == NULL) {
        return -1;
    }

    g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (g_renderer == NULL) {
        g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);
    }
    if (g_renderer == NULL) {
        return -1;
    }

    for (i = 0; i < PIT_SCREEN_COUNT; i++) {
        g_texture[i] = SDL_CreateTexture(
            g_renderer,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING,
            PIT_SCREEN_WIDTH,
            PIT_SCREEN_HEIGHT);
        if (g_texture[i] == NULL) {
            return -1;
        }
        SDL_SetTextureBlendMode(g_texture[i], SDL_BLENDMODE_BLEND);
    }

    return 0;
}

void pit_platform_video_shutdown(void)
{
    int i;

    for (i = 0; i < PIT_SCREEN_COUNT; i++) {
        if (g_texture[i] != NULL) {
            SDL_DestroyTexture(g_texture[i]);
            g_texture[i] = NULL;
        }
    }
    if (g_renderer != NULL) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = NULL;
    }
    if (g_window != NULL) {
        SDL_DestroyWindow(g_window);
        g_window = NULL;
    }
    SDL_Quit();
}

void pit_platform_present(const pit_frame *frame, const pit_present_info *info)
{
    int scale = (info->scale > 0) ? info->scale : g_scale;
    int width;
    int height;
    int i;

    compute_window_size(info->layout, scale, &width, &height);
    SDL_SetWindowSize(g_window, width, height);

    SDL_SetRenderDrawColor(g_renderer, 0x10, 0x10, 0x18, 255);
    SDL_RenderClear(g_renderer);

    for (i = 0; i < PIT_SCREEN_COUNT; i++) {
        SDL_UpdateTexture(
            g_texture[i],
            NULL,
            frame->screen[i].data,
            PIT_SCREEN_WIDTH * (int)sizeof(pit_pixel));
    }

    for (i = 0; i < PIT_SCREEN_COUNT; i++) {
        SDL_Rect dst = screen_rect(i, info->layout, scale);
        int alpha = (info->layout == PIT_LAYOUT_OVERLAY && i == PIT_SCREEN_ACTION) ? 150 : 255;
        SDL_SetTextureAlphaMod(g_texture[i], (unsigned char)alpha);
        SDL_RenderCopy(g_renderer, g_texture[i], NULL, &dst);
    }

    for (i = 0; i < PIT_SCREEN_COUNT; i++) {
        SDL_Rect dst = screen_rect(i, info->layout, scale);
        int focused = (info->focus == (pit_screen_id)i);
        SDL_Rect border;

        border.x = dst.x + 2;
        border.y = dst.y + 2;
        border.w = dst.w - 4;
        border.h = dst.h - 4;

        if (focused) {
            SDL_SetRenderDrawColor(g_renderer, 0xFF, 0xD0, 0x40, 255);
        } else {
            SDL_SetRenderDrawColor(g_renderer, 0x44, 0x44, 0x50, 255);
        }
        SDL_RenderDrawRect(g_renderer, &border);
    }

    SDL_RenderPresent(g_renderer);
}

void pit_platform_poll(void)
{
    SDL_Event event;

    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            g_quit = 1;
        } else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_ESCAPE:
                g_quit = 1;
                break;
            case SDL_SCANCODE_F1:
                pit_video_cycle_layout();
                break;
            case SDL_SCANCODE_F2:
                pit_video_cycle_focus_mode();
                break;
            case SDL_SCANCODE_1:
                pit_video_set_layout(PIT_LAYOUT_STACKED);
                break;
            case SDL_SCANCODE_2:
                pit_video_set_layout(PIT_LAYOUT_SIDE_BY_SIDE);
                break;
            case SDL_SCANCODE_3:
                pit_video_set_layout(PIT_LAYOUT_OVERLAY);
                break;
            default:
                break;
            }
        }
    }
}

int pit_platform_quit_requested(void)
{
    return g_quit;
}

void pit_platform_delay_ms(unsigned int ms)
{
    SDL_Delay((Uint32)ms);
}
