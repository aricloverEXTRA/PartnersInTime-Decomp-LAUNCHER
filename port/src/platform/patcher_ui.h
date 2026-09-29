#ifndef PIT_PLATFORM_PATCHER_UI_H
#define PIT_PLATFORM_PATCHER_UI_H

/*
 * Entry point for the graphical patcher. Optional arguments are the source ROM
 * and the output ROM, so a path can be supplied on the command line and then
 * edited in the UI.
 */
int pit_patcher_ui_main(int argc, char **argv);

#endif
