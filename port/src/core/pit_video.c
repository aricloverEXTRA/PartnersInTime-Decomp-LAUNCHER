#include "core/pit_video.h"

static pit_layout_mode g_layout     = PIT_LAYOUT_STACKED;
static pit_focus_mode  g_focus_mode = PIT_FOCUS_AUTO;
static pit_screen_id   g_active     = PIT_SCREEN_ENGINE;

void pit_video_init(void)
{
    g_layout     = PIT_LAYOUT_STACKED;
    g_focus_mode = PIT_FOCUS_AUTO;
    g_active     = PIT_SCREEN_ENGINE;
}

void pit_video_set_layout(pit_layout_mode mode)
{
    if (mode >= 0 && mode < PIT_LAYOUT_COUNT) {
        g_layout = mode;
    }
}

pit_layout_mode pit_video_layout(void)
{
    return g_layout;
}

void pit_video_cycle_layout(void)
{
    g_layout = (pit_layout_mode)((g_layout + 1) % PIT_LAYOUT_COUNT);
}

void pit_video_set_focus_mode(pit_focus_mode mode)
{
    if (mode >= 0 && mode < PIT_FOCUS_COUNT) {
        g_focus_mode = mode;
    }
}

pit_focus_mode pit_video_focus_mode(void)
{
    return g_focus_mode;
}

void pit_video_cycle_focus_mode(void)
{
    g_focus_mode = (pit_focus_mode)((g_focus_mode + 1) % PIT_FOCUS_COUNT);
}

void pit_video_set_active_screen(pit_screen_id screen)
{
    if (screen >= 0 && screen < PIT_SCREEN_COUNT) {
        g_active = screen;
    }
}

pit_screen_id pit_video_focus(void)
{
    if (g_focus_mode == PIT_FOCUS_FORCE_ENGINE) {
        return PIT_SCREEN_ENGINE;
    }
    if (g_focus_mode == PIT_FOCUS_FORCE_ACTION) {
        return PIT_SCREEN_ACTION;
    }
    return g_active;
}

void pit_video_step(void)
{
}

void pit_video_fill_info(pit_present_info *out)
{
    out->layout = g_layout;
    out->focus  = pit_video_focus();
    out->scale  = 1;
}
