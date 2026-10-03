/*
 * Write buffer wait (ARM9 resident, 0x0203A034-0x0203A040).
 *
 * The ARM9 write buffer has to drain before this core reads memory the other
 * CPU may have just written.
 */

#include <nitro.h>

void DC_WaitWriteBufferEmpty(void)
{
    u32 zero = 0;
    /* CP15 maintenance opc7/CRn7/CRm10/opc1=4: wait for write buffer empty. */
    asm { mcr p15, 0, zero, c7, c10, 4 }
}