#ifndef PIT_CORE_PIT_VIDEO_H
#define PIT_CORE_PIT_VIDEO_H

#include "core/pit.h"

void pit_video_init(void);

void pit_video_set_layout(pit_layout_mode mode);
pit_layout_mode pit_video_layout(void);
void pit_video_cycle_layout(void);

void pit_video_set_focus_mode(pit_focus_mode mode);
pit_focus_mode pit_video_focus_mode(void);
void pit_video_cycle_focus_mode(void);

void pit_video_set_active_screen(pit_screen_id screen);
pit_screen_id pit_video_focus(void);

void pit_video_step(void);
void pit_video_fill_info(pit_present_info *out);

#endif
