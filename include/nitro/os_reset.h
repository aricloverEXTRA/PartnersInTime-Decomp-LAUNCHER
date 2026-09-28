#ifndef NITRO_OS_RESET_H
#define NITRO_OS_RESET_H

/*
 * Soft reset.
 */

#include <nitro.h>

void OS_InitReset(void);
void OS_ResetSystem(u32 parameter);
void OSi_SendResetCommand(u32 command);

#endif
