#ifndef NITRO_OS_LOCK_H
#define NITRO_OS_LOCK_H

/*
 * The word-based locks the SDK arbitrates shared hardware with, including the
 * cartridge and card locks that keep the two processors off the same bus.
 */
#include <nitro/os_sync.h>

typedef struct OsLockWord {
    volatile u32 flag;
    volatile u16 owner;
    volatile u16 extension;
} OsLockWord;
typedef void (*OsLockCallback)(void);

void OSi_SpinWaitLock(void);
int OS_GetLockID(void);
u16 OS_ReadOwnerOfLockWord(const OsLockWord *lock);
void OSi_ReleaseCardBus(void);
void OSi_AllocateCardBus(void);
void OSi_ReleaseCartridgeBus(void);
void OSi_AllocateCartridgeBus(void);
int OS_UnlockCard(u16 id);
int OS_LockCard(u16 id);
int OS_TryLockCartridge(u16 id);
int OS_UnlockCartridge(u16 id);
int OS_LockCartridge(u16 id);
int OSi_TryLockWord(u16 id, OsLockWord *lock, OsLockCallback callback, int disable_fiq);
int OS_UnlockWord(u16 id, OsLockWord *lock, OsLockCallback callback);
int OSi_UnlockWord(u16 id, OsLockWord *lock, OsLockCallback callback, int disable_fiq);
int OS_LockWord(u16 id, OsLockWord *lock, OsLockCallback callback);
int OSi_LockWord(u16 id, OsLockWord *lock, OsLockCallback callback, int disable_fiq);
void OS_InitLock(void);

#endif
