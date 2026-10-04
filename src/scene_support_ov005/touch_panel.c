/*
 * Overlay 5 touch panel input (overlay 5, 0x020686CC-0x02068908).
 *
 * Reads the auto-sampling buffer the touch driver fills and turns it into the
 * frame's touch state. Also holds the texture-object helpers that share this
 * address range in the original build.
 */

#include <game/battle_scene.h>
#include <nitro.h>
#include <nitro/tp.h>
#include <game/heap.h>

/* Touch-panel sampling state owned by this overlay. */
extern u8 data_ov005_0206a318, data_ov005_0206a31c, data_ov005_0206a320;
extern TpData data_ov005_0206a324[9];
extern u8 data_ov005_0206a36c[94];
extern void *data_ov005_0206a02c[];

extern void MI_CpuFill8(void *destination, u8 value, u32 size);
extern void func_0200c9c8(void *object, int mode);

void func_ov005_02068758(void);
void func_ov005_0206873c(int first, int second);

/* An overlay-5 display object whose graphics are decoded into an owned buffer. */
typedef struct Overlay5TextureObject {
    void **vtable;
    u8 unknown_04[16];
    const void *resource;
    u8 unknown_18[4];
    void *buffer;
    u8 unknown_20[292];
    u8 mode;
} Overlay5TextureObject;

void func_ov005_020688cc(Overlay5TextureObject *object, int copy_flag) {
    u32 size = BattleModel_GetScreenTextureConversionSize(0, copy_flag, object->resource);
    object->buffer = size ? GameHeap_NewArray(size, 0, 0, 1) : 0;
}

Overlay5TextureObject *func_ov005_020688a0(Overlay5TextureObject *object) {
    object->vtable = data_ov005_0206a02c;
    BattleModelController_DestroyBase((BattleModel *)object);
    GameHeap_Delete(object);
    return object;
}

void func_ov005_0206889c(void) {
}

void func_ov005_02068898(void) {
}

void func_ov005_02068894(void) {
}

void func_ov005_02068868(Overlay5TextureObject *object) {
    if (!object->mode) return;
    BattleModel_UpdateSpriteTexture((BattleModel *)object, object->mode);
}

void func_ov005_0206883c(Overlay5TextureObject *object) {
    if (!object->mode) return;
    func_0200c9c8(object, object->mode);
}

Overlay5TextureObject *func_ov005_02068818(Overlay5TextureObject *object) {
    object->vtable = data_ov005_0206a02c;
    BattleModelController_DestroyBase((BattleModel *)object);
    return object;
}

void func_ov005_020687b8(void) {
    TpCalibration calibration;
    TP_Init();
    TP_GetUserInfo(&calibration);
    TP_SetCalibrateParam(&calibration);
    TP_RequestAutoSamplingStartAsync(0, 4, data_ov005_0206a324, 9);
    TP_WaitBusy(2);
    TP_CheckError(2);
    func_ov005_02068758();
    func_ov005_0206873c(12, 4);
}

void func_ov005_02068790(void) {
    TP_RequestAutoSamplingStopAsync();
    TP_WaitBusy(4);
    TP_CheckError(4);
}

void func_ov005_02068758(void) {
    MI_CpuFill8(data_ov005_0206a36c, 0, sizeof(data_ov005_0206a36c));
    data_ov005_0206a320 = 0;
}

void func_ov005_0206873c(int first, int second) {
    data_ov005_0206a318 = first;
    data_ov005_0206a31c = second;
}

/* Calibrates the four sampling slots around the latest auto-sampling index into
 * this overlay's own sample array, keeping only slots whose raw reading is
 * empty. The write cursor lives in data_ov005_0206a36c[0x50] and steps in
 * TpData-sized strides. */
void func_ov005_020686cc(void) {
    int index = TP_GetLatestIndexInAuto();
    u8 *out = data_ov005_0206a36c;
    int i;

    data_ov005_0206a36c[0x50] = 0;

    for (i = 0; i < 4; i++) {
        int slot = index - 3 + i;

        if (slot < 0) {
            slot += 9;
        }

        if (data_ov005_0206a324[slot].validity == 0) {
            TP_GetCalibratedPoint((TpData *)(out + data_ov005_0206a36c[0x50] * 8),
                                  (const TpData *)&data_ov005_0206a324[slot]);
            data_ov005_0206a36c[0x50]++;
        }
    }
}
