/*
 * Thin entry point for the graphical Windows patcher.
 *
 * Kept separate from src/patcher_main.c so the headless driver and the UI stay
 * independently buildable: the packaging script and the round-trip test use the
 * headless binary and must not require SDL.
 */

#include "platform/patcher_ui.h"

int main(int argc, char **argv)
{
    return pit_patcher_ui_main(argc, argv);
}
