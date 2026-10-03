/*
 * RTC busy wait (ARM9 resident, 0x02041DAC-0x02041DC4).
 *
 * Spins on the RTC driver's sync-result word until the hardware clears it.
 * The poll is one assembly fragment because MWCC materialises the literal-pool
 * base in r1, where native keeps it in the ip scratch register, and it hoists
 * the first comparison out of a C loop.
 */

#include <nitro.h>

void RTCi_WaitBusy(void)
{
    asm {
        ldr ip, =0x02064D24
    wait:
        ldr r0, [ip]
        cmp r0, #1
        beq wait
    }
}