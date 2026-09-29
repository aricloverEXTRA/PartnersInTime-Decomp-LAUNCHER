/* Status-page row cleanup (overlay 7, 0x02078478-0x020784A0). */
#include <game/sprite_output.h>
#include <game/pause_navigation.h>
extern GameSpriteAllocation data_ov007_020908d0;
void func_ov005_020663d8(int group);
void PauseStatusPage_Release(void) {
    /* The task sweep releases these rows after the current update finishes. */
    func_ov005_020663d8(2);
    GameSpriteAllocation_Unlink(&data_ov007_020908d0);
}
