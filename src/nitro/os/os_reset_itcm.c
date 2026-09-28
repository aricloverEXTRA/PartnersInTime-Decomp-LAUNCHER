/* Reset code kept in ITCM while the resident ARM9 image is overwritten.
 * Native range: 0x01FF83A0..0x01FF84C0.
 */
#include <nitro/os_sync.h>
#include <nitro/os_reset.h>

extern u16 data_02063024;
extern void OSi_ReloadRomData(void);
extern void func_01ff81ac(void);
extern void func_01ff8270(u32 offset, void *destination, u32 size);
extern void DC_StoreAll(void);
extern void DC_InvalidateAll(void);
extern void IC_InvalidateAll(void);
extern void DC_WaitWriteBufferEmpty(void);

void OSi_DoResetSystem(void)
{
    /* The ARM7 FIFO callback changes this flag while we wait. */
    while (!*(vu16 *)&data_02063024) {}
    *(vu16 *)0x04000208 = 0;
    OSi_ReloadRomData();
    func_01ff81ac();
}

void OSi_ReloadRomData(void)
{
    u32 base = *(u32 *)0x027ffc2c;
    u32 arm9_offset, arm9_destination, arm9_size;
    u32 arm7_offset, arm7_destination, arm7_size;
    u32 state;

    if (base >= 0x8000)
        func_01ff8270(base, (void *)0x027ffe00, 0x160);
    /* Cache both header triples before replacing the resident images. */
    arm9_offset = *(u32 *)0x027ffe20;
    arm9_destination = *(u32 *)0x027ffe28;
    arm9_size = *(u32 *)0x027ffe2c;
    arm7_offset = *(u32 *)0x027ffe30;
    arm7_destination = *(u32 *)0x027ffe38;
    arm7_size = *(u32 *)0x027ffe3c;
    state = OS_DisableInterrupts();
    DC_StoreAll();
    DC_InvalidateAll();
    OS_RestoreInterrupts(state);
    IC_InvalidateAll();
    DC_WaitWriteBufferEmpty();
    arm9_offset += base;
    /* The secure-area prefix is already present; card reads start at 0x8000. */
    if (arm9_offset < 0x8000) {
        u32 skipped = 0x8000 - arm9_offset;
        arm9_destination += skipped;
        arm9_size -= skipped;
        arm9_offset = 0x8000;
    }
    /* Complete both ROM offsets before assigning the next call arguments. */
    asm { add arm7_offset, arm7_offset, base }
    func_01ff8270(arm9_offset, (void *)arm9_destination, arm9_size);
    func_01ff8270(arm7_offset, (void *)arm7_destination, arm7_size);
}
