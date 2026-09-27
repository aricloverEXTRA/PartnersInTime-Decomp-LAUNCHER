#ifndef PIT_CREDITS_TRANSITION_INTERNAL_H
#define PIT_CREDITS_TRANSITION_INTERNAL_H

#include "frontend_scene_internal.h"

#define CREDITS_TRANSITION data_ov006_0207c594
extern CreditsMotion data_ov006_020847d0[1024];

/* X and Y views into the same table of 16-byte layout records. */
extern const s32 data_ov006_0207bff0[][4], data_ov006_0207bff4[][4];

int func_ov006_02077ce8(CreditsMotion *motion);

/* Staging buffers for the next 256x128 texture and its 256-color palette. */
typedef struct CreditsPalette { u16 colors[256]; } CreditsPalette;
typedef char CreditsPaletteSizeCheck[sizeof(CreditsPalette) == 512 ? 1 : -1];
extern u8 data_ov006_0207c5cc[32768];
extern CreditsPalette data_ov006_020845cc;
void CreditsCloud_Update(MenuElement *element);
void CreditsBackground_DrawGradient(void);
void func_ov006_020798c8(void);
void CreditsStar_Update(MenuElement *element);
void CreditsImage_LoadNext(MenuElement *element);
void CreditsMusic_Update(MenuElement *element);
void CreditsMusic_StopAfterDelay(MenuElement *element);
void func_ov006_02078938(MenuElement *element);
void CreditsImage_UploadTextureChunk(MenuElement *element);
void func_ov005_0206650c(MenuElement *element);
u32 func_ov005_02066ed4(Overlay5Archive *, u8, u16, void **, int);
int func_ov005_02067238(Overlay5Archive *, u32, u32);
void GameResource_Move16(const s16 *, s16 *, u32);

#endif
