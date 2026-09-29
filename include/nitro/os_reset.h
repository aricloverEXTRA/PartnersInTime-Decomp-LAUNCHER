#ifndef NITRO_OS_RESET_H
#define NITRO_OS_RESET_H

/*
 * Soft reset.
 */

#include <nitro.h>

void OS_InitReset(void);
/* ITCM entry; remains executable while the resident image is reloaded. */
void OSi_DoResetSystem(void);
void OS_ResetSystem(u32 parameter);
void OSi_SendResetCommand(u32 command);

#endif
