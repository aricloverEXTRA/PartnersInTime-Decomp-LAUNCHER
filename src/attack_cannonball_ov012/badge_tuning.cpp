/* Badge-driven adjustment to the Cannonballers motion and timing parameters. */
#include "motion_internal.h"
extern "C" int data_ov012_020c5a08, data_ov012_020c5a10;

extern "C" void Overlay12Attack_ApplyBadgeOffsets(void)
{
    /* Preserve the native binary64 conversion and addition. */
    data_ov012_020c5a04 += -128.0;
    data_ov012_020c5a10 += 8;
    data_ov012_020c5a08 += 4;
}
