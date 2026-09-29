#ifndef PIT_PLATFORM_PIT_PLATFORM_H
#define PIT_PLATFORM_PIT_PLATFORM_H

#include "core/pit.h"

int  pit_platform_video_init(const char *title);
void pit_platform_video_shutdown(void);
void pit_platform_present(const pit_frame *frame, const pit_present_info *info);
void pit_platform_poll(void);
int  pit_platform_quit_requested(void);
void pit_platform_delay_ms(unsigned int ms);

#endif
