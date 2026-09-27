/*
 * Console type (ARM9 resident, 0x02039A60-0x02039BAC).
 *
 * Reports which DS model the game is running on.
 */

#include <nitro/os_lock.h>

extern u32 data_02059d90;
u32 OSi_ReadCartridgeSignatureType(void);
int func_02039bac(void);
int func_02039a2c(void);

u32 func_02039b34(void) {
    if (data_02059d90 == 0xffffffff) {
        u32 cartridge = OSi_ReadCartridgeSignatureType();
        u32 result;
        if (func_02039bac())
            result = cartridge | 0x10000000;
        else if (func_02039a2c())
            result = cartridge | 0x40000000;
        else if (cartridge & 0x01000000)
            result = cartridge | 0x20000000;
        else
            result = cartridge | 0x80000000;
        data_02059d90 = result | *(vu16 *)0x027ffffa;
    }
    return data_02059d90;
}

/* Classify the two words at the GBA bus base while access belongs to ARM9.
 * An existing ARM9 owner permits reading, but the loop exits only after this
 * call obtains and releases the lock itself. The allocated lock ID is retained. */
u32 OSi_ReadCartridgeSignatureType(void)
{
    u32 type;
    int finished = 0;
    u16 lock_id = OS_GetLockID();
    type = 0;
    do {
        int acquired = -1;
        u32 interrupts = OS_DisableInterrupts();
        if ((OS_ReadOwnerOfLockWord((const OsLockWord *)0x027FFFE8) & 0x40) ||
            (acquired = OS_TryLockCartridge(lock_id)) == 0) {
            /* Little-endian "NINTENDO" signature. */
            if (*(vu32 *)0x08000000 == 0x544E494E &&
                *(vu32 *)0x08000004 == 0x4F444E45)
                type = 0x01000000;
            else
                type = 0x02000000;
            if (!acquired) {
                OS_UnlockCartridge(lock_id);
                finished = 1;
            }
        }
        OS_RestoreInterrupts(interrupts);
    } while (!finished);
    return type;
}
