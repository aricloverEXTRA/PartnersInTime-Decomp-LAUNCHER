/*
 * Battle init thunk (ov002, 0x0206A008-0x0206A02C).
 * Calls two resident init functions with argument 1.
 */

#include <nitro.h>

extern void func_020090b8(unsigned int);
extern void GameSpritePalette_UploadScreen(unsigned int);

void func_ov002_0206a008(void)
{
    func_020090b8(1);
    GameSpritePalette_UploadScreen(1);
}